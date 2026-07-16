#include "rendering/passes/environment/VolumetricSkyPass.h"

#include <algorithm>
#include <cmath>

#include "rendering/passes/environment/SkyboxPass.h"

namespace
{
int scaledExtent(int extent, float scale)
{
    return std::max(1, static_cast<int>(std::ceil(
        static_cast<float>(std::max(extent, 1)) * scale)));
}
}

VolumetricSkyPass::VolumetricSkyPass(
    Camera& camera,
    SceneRenderResources& resources,
    SceneRenderConfig& config,
    LightSettings& lightSettings,
    int width,
    int height)
    : camera_(camera)
    , resources_(resources)
    , config_(config)
    , lightSettings_(lightSettings)
    , cloudAccelerationPass_(camera, resources, config, lightSettings)
    , lowResolutionTarget_(
          scaledExtent(width, 0.5f),
          scaledExtent(height, 0.5f))
    , targetWidth_(scaledExtent(width, 0.5f))
    , targetHeight_(scaledExtent(height, 0.5f))
{}

void VolumetricSkyPass::render(
    int width,
    int height,
    Framebuffer& destination,
    Screenquad& screenQuad,
    GpuProfiler& profiler)
{
    if (!config_.enableSkybox || !resources_.shaderLibrary)
        return;

    if (!shouldRenderVolumetricClouds(config_))
    {
        destination.bind();
        ScopedGpuPass timer(profiler, "Skybox");
        SkyboxPass::renderSkyboxPass(
            camera_, resources_, config_, lightSettings_, width, height);
        destination.unbind();
        return;
    }

    cloudAccelerationPass_.bindForCloudShader(
        resources_.shaderLibrary->cubemap);

    ensureRenderTargetSize(width, height);
    lowResolutionTarget_.bind();
    glViewport(0, 0, targetWidth_, targetHeight_);
    glClearColor(
        config_.skyTopColor.r,
        config_.skyTopColor.g,
        config_.skyTopColor.b,
        1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    {
        ScopedGpuPass timer(profiler, "Volumetric Sky");
        SkyboxPass::renderSkyboxPass(
            camera_, resources_, config_, lightSettings_,
            targetWidth_, targetHeight_);
    }

    {
        ScopedGpuPass timer(profiler, "Sky Upsample");
        composite(width, height, destination, screenQuad);
    }
    destination.unbind();
}

void VolumetricSkyPass::updateCloudAcceleration(
    Screenquad& screenQuad,
    GpuProfiler& profiler)
{
    if (!shouldRenderVolumetricClouds(config_) || !resources_.shaderLibrary)
        return;
    cloudAccelerationPass_.update(screenQuad, profiler);
}

void VolumetricSkyPass::resize(int width, int height)
{
    ensureRenderTargetSize(width, height);
}

void VolumetricSkyPass::ensureRenderTargetSize(int width, int height)
{
    const float scale = std::clamp(config_.cloudRenderScale, 0.25f, 1.0f);
    const int requestedWidth = scaledExtent(width, scale);
    const int requestedHeight = scaledExtent(height, scale);
    if (requestedWidth == targetWidth_ && requestedHeight == targetHeight_)
        return;

    targetWidth_ = requestedWidth;
    targetHeight_ = requestedHeight;
    lowResolutionTarget_.resize(targetWidth_, targetHeight_);
}

void VolumetricSkyPass::composite(
    int width,
    int height,
    Framebuffer& destination,
    Screenquad& screenQuad)
{
    if (!resources_.shaderLibrary)
        return;

    destination.bind();
    glViewport(0, 0, width, height);

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    Shader& shader = resources_.shaderLibrary->skyComposite;
    shader.use();
    shader.setInt("skyColor", 0);
    shader.setInt("skyBright", 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lowResolutionTarget_.getTextureID(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, lowResolutionTarget_.getTextureID(1));
    screenQuad.draw();

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    if (!depthTestWasEnabled)
        glDisable(GL_DEPTH_TEST);
    if (blendWasEnabled)
        glEnable(GL_BLEND);
}
