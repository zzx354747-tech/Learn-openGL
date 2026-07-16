#include "rendering/passes/lighting/VolumetricLightPass.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "rendering/uniforms/TemporalJitter.h"

namespace
{
int scaledExtent(int extent, float scale)
{
    return std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(std::max(extent, 1)) * scale)));
}

glm::mat4 inverseViewProjection(
    const Camera& camera,
    int width,
    int height)
{
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(std::max(width, 1)) /
            static_cast<float>(std::max(height, 1)),
        0.1f,
        20000.0f);
    projection = TemporalJitter::apply(projection, width, height);
    return glm::inverse(projection * camera.GetViewMatrix());
}
}

VolumetricLightPass::VolumetricLightPass(
    Camera& camera,
    ShadowResources& shadowResources,
    SceneRenderResources& resources,
    SceneRenderConfig& config,
    SceneRenderState& state,
    LightSettings& lightSettings,
    int width,
    int height)
    : camera_(camera)
    , shadowResources_(shadowResources)
    , resources_(resources)
    , config_(config)
    , state_(state)
    , lightSettings_(lightSettings)
    , halfResolutionTarget_(
          scaledExtent(width, 0.5f),
          scaledExtent(height, 0.5f))
    , depthCopyTarget_(std::max(width, 1), std::max(height, 1))
    , targetWidth_(scaledExtent(width, 0.5f))
    , targetHeight_(scaledExtent(height, 0.5f))
    , fullWidth_(std::max(width, 1))
    , fullHeight_(std::max(height, 1))
{}

void VolumetricLightPass::render(
    int width,
    int height,
    Framebuffer& destination,
    Screenquad& screenQuad,
    const CloudAccelerationPass& cloudAccelerationPass,
    GpuProfiler& profiler)
{
    if (!shouldRenderGodRays(config_) || !config_.enableDirectionalLight ||
        !resources_.shaderLibrary || !shadowResources_.shadowMap)
    {
        return;
    }

    ensureTargetSize(width, height);
    {
        ScopedGpuPass timer(profiler, "Volumetric Depth Copy");
        copySceneDepth(width, height, destination);
    }
    {
        ScopedGpuPass timer(profiler, "Volumetric Light March");
        rayMarch(width, height, screenQuad, cloudAccelerationPass);
    }
    {
        ScopedGpuPass timer(profiler, "Volumetric Light Upsample");
        bilateralComposite(width, height, destination, screenQuad);
    }
    ++frameIndex_;
}

void VolumetricLightPass::resize(int width, int height)
{
    ensureTargetSize(width, height);
}

void VolumetricLightPass::ensureTargetSize(int width, int height)
{
    const float scale = std::clamp(
        config_.volumetricLightRenderScale, 0.25f, 1.0f);
    const int requestedWidth = scaledExtent(width, scale);
    const int requestedHeight = scaledExtent(height, scale);
    if (requestedWidth != targetWidth_ || requestedHeight != targetHeight_)
    {
        targetWidth_ = requestedWidth;
        targetHeight_ = requestedHeight;
        halfResolutionTarget_.resize(targetWidth_, targetHeight_);
    }

    const int requestedFullWidth = std::max(width, 1);
    const int requestedFullHeight = std::max(height, 1);
    if (requestedFullWidth != fullWidth_ || requestedFullHeight != fullHeight_)
    {
        fullWidth_ = requestedFullWidth;
        fullHeight_ = requestedFullHeight;
        depthCopyTarget_.resize(fullWidth_, fullHeight_);
    }
}

void VolumetricLightPass::copySceneDepth(
    int width,
    int height,
    Framebuffer& source)
{
    GLint previousReadFramebuffer = 0;
    GLint previousDrawFramebuffer = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, source.getFBO());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, depthCopyTarget_.getFBO());
    glBlitFramebuffer(
        0, 0, width, height,
        0, 0, fullWidth_, fullHeight_,
        GL_DEPTH_BUFFER_BIT,
        GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
}

void VolumetricLightPass::rayMarch(
    int width,
    int height,
    Screenquad& screenQuad,
    const CloudAccelerationPass& cloudAccelerationPass)
{
    (void)cloudAccelerationPass;
    halfResolutionTarget_.bind();
    glViewport(0, 0, targetWidth_, targetHeight_);

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& shader = resources_.shaderLibrary->volumetricLight;
    shader.use();
    shader.setInt("sceneDepth", 0);
    shader.setInt("directionalShadowMap", 1);
    shader.setMat4(
        "inverseViewProjection",
        inverseViewProjection(camera_, width, height));
    shader.setMat4("lightSpaceMatrix", state_.dirLightSpaceMatrix);
    shader.setMat4("cloudShadowMatrix", state_.cloudShadowMatrix);
    shader.setVec3("cameraPos", camera_.Getposition());
    shader.setVec3(
        "towardSun",
        glm::normalize(-lightSettings_.sunDirection));
    const float fallbackSunTransmission =
        calculateCloudSunTransmission(config_);
    const glm::vec3 volumeTint =
        lightSettings_.sunDiffuse * lightSettings_.sunIntensity *
        lightSettings_.sunIntensityScale * config_.godRayColor;
    const float volumeTintLuminance = glm::dot(
        volumeTint, glm::vec3(0.2126f, 0.7152f, 0.0722f));
    const glm::vec3 volumeChromaticity = volumeTintLuminance > 0.0001f
        ? volumeTint / volumeTintLuminance
        : glm::vec3(1.0f);
    // Surface BRDF lighting uses intentionally large scene-light values. The
    // participating medium has its own radiance scale, so looking toward the
    // sun cannot turn the shaft pass into a second lens-glare implementation.
    constexpr float VolumeReferenceRadiance = 3.0f;
    shader.setVec3(
        "sunRadiance",
        volumeChromaticity * VolumeReferenceRadiance *
            config_.volumetricLightIntensity * config_.godRayIntensity *
            config_.daylightFactor);
    const float requestedSigmaT =
        std::max(config_.volumetricLightExtinction, 0.000001f);
    const float distanceToCloudBase = std::max(
        config_.cloudBaseHeight - camera_.Getposition().y, 500.0f);
    // Keep roughly 30% view transmittance at the cloud base. A fixed
    // 500-m extinction applied to a 4-km storm layer erased the upper part of
    // every shaft before it reached the camera.
    const float cloudRangeSigmaT =
        -std::log(0.30f) / distanceToCloudBase;
    const float effectiveSigmaT = std::min(
        requestedSigmaT, cloudRangeSigmaT);
    const float scatteringAlbedo = std::clamp(
        config_.volumetricLightScattering / requestedSigmaT,
        0.0f, 0.95f);
    shader.setFloat(
        "sigmaT",
        effectiveSigmaT);
    shader.setFloat(
        "sigmaS",
        effectiveSigmaT * scatteringAlbedo);
    shader.setFloat(
        "phaseG",
        std::clamp(config_.volumetricLightAnisotropy, 0.5f, 0.76f));
    shader.setFloat(
        "maxDistance",
        std::max(config_.volumetricLightMaxDistance, 1.0f));
    shader.setInt(
        "stepCount",
        std::clamp(config_.volumetricLightSteps, 16, 64));
    shader.setInt("frameIndex", static_cast<int>(frameIndex_ & 1023u));
    shader.setFloat(
        "shadowBias",
        std::max(config_.directionalShadowBiasMin, 0.00001f));
    shader.setFloat("cloudBaseHeight", config_.cloudBaseHeight);
    shader.setFloat("cloudThickness", config_.cloudThickness);
    shader.setFloat(
        "cloudFallbackSunTransmission", fallbackSunTransmission);
    shader.setBool(
        "hasCloudOpticalDepthMap",
        shadowResources_.cloudOpticalDepthTexture != 0 &&
        shadowResources_.cloudTransmittanceTexture != 0);
    shader.setInt("cloudOpticalDepthMap", 2);
    shader.setInt("cloudTransmittanceMap", 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthCopyTarget_.getDepthTextureID());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(
        GL_TEXTURE_2D,
        shadowResources_.shadowMap->getDepthMapTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(
        GL_TEXTURE_2D,
        shadowResources_.cloudOpticalDepthTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(
        GL_TEXTURE_2D,
        shadowResources_.cloudTransmittanceTexture);

    screenQuad.draw();

    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    if (blendWasEnabled)
        glEnable(GL_BLEND);
    halfResolutionTarget_.unbind();
}

void VolumetricLightPass::bilateralComposite(
    int width,
    int height,
    Framebuffer& destination,
    Screenquad& screenQuad)
{
    destination.bind();
    glViewport(0, 0, width, height);

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    GLint previousBlendSourceRgb = GL_ONE;
    GLint previousBlendDestinationRgb = GL_ZERO;
    GLint previousBlendSourceAlpha = GL_ONE;
    GLint previousBlendDestinationAlpha = GL_ZERO;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSourceRgb);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDestinationRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDestinationAlpha);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ONE, GL_ONE);

    Shader& shader = resources_.shaderLibrary->volumetricLightComposite;
    shader.use();
    shader.setInt("lowResolutionScattering", 0);
    shader.setInt("lowResolutionDistance", 1);
    shader.setInt("sceneDepth", 2);
    shader.setMat4(
        "inverseViewProjection",
        inverseViewProjection(camera_, width, height));
    shader.setVec3("cameraPos", camera_.Getposition());
    shader.setFloat(
        "maxDistance",
        std::max(config_.volumetricLightMaxDistance, 1.0f));
    shader.setFloat(
        "depthSigma",
        std::max(config_.volumetricLightDepthSigma, 0.0001f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, halfResolutionTarget_.getTextureID(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, halfResolutionTarget_.getTextureID(1));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, depthCopyTarget_.getDepthTextureID());
    screenQuad.draw();

    glBlendFuncSeparate(
        previousBlendSourceRgb,
        previousBlendDestinationRgb,
        previousBlendSourceAlpha,
        previousBlendDestinationAlpha);
    if (!blendWasEnabled)
        glDisable(GL_BLEND);
    glDepthMask(depthWriteWasEnabled);
    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    destination.unbind();
}
