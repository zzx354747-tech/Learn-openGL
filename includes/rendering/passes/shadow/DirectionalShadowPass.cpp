#include "rendering/passes/shadow/DirectionalShadowPass.h"
#include "rendering/debug/GpuProfiler.h"
#include "rendering/assets/mesh/TerrainMesh.h"

#include <algorithm>
#include <cmath>

namespace
{
void createR16FTarget(
    unsigned int& framebuffer,
    unsigned int& texture,
    int resolution,
    bool mipmapped,
    float clearValue)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_R16F,
        resolution, resolution, 0,
        GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, texture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::CLOUD_SHADOW::R16F framebuffer incomplete"
                  << std::endl;
    glClearColor(clearValue, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (mipmapped)
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
}

DirectionalShadowPass::DirectionalShadowPass(
    DirectionalShadowMap& shadowMap,
    Shader& shadowShader,
    Shader& cloudOpticalDepthShader,
    Shader& cloudOpticalDepthBlurShader,
    Shader& cloudOpticalDepthToTransmittanceShader,
    SphereDrawer& sphereDrawer,
    ModelDrawer& modelDrawer,
    Camera& camera,
    SceneRenderState& state,
    LightSettings& lightSettings,
    SceneRenderConfig& config,
    ResourceRegistry& registry,
    ResourceHandle shadowMapHandle)
    : registry(registry)
    , shadowMapHandle(shadowMapHandle)
    , shadowMap(shadowMap)
    , shadowShader(shadowShader)
    , cloudOpticalDepthShader(cloudOpticalDepthShader)
    , cloudOpticalDepthBlurShader(cloudOpticalDepthBlurShader)
    , cloudOpticalDepthToTransmittanceShader(
          cloudOpticalDepthToTransmittanceShader)
    , sphereDrawer(sphereDrawer)
    , modelDrawer(modelDrawer)
    , camera(camera)
    , state(state)
    , lightSettings(lightSettings)
    , config(config)
{
    createCloudTextures();
}

DirectionalShadowPass::~DirectionalShadowPass()
{
    glDeleteFramebuffers(1, &rawCloudFramebuffer);
    glDeleteFramebuffers(1, &filteredCloudFramebuffer);
    glDeleteFramebuffers(1, &cloudTransmittanceFramebuffer);
    glDeleteTextures(1, &rawCloudOpticalDepthTexture);
    glDeleteTextures(1, &filteredCloudOpticalDepthTexture);
    glDeleteTextures(1, &cloudTransmittanceTextureId);
}

void DirectionalShadowPass::createCloudTextures()
{
    createR16FTarget(
        rawCloudFramebuffer,
        rawCloudOpticalDepthTexture,
        CloudShadowResolution,
        false,
        0.0f);
    createR16FTarget(
        filteredCloudFramebuffer,
        filteredCloudOpticalDepthTexture,
        CloudShadowResolution,
        false,
        0.0f);
    createR16FTarget(
        cloudTransmittanceFramebuffer,
        cloudTransmittanceTextureId,
        CloudShadowResolution,
        true,
        1.0f);
}

void DirectionalShadowPass::render(
    Screenquad& screenQuad,
    GpuProfiler& profiler)
{
    state.dirLightSpaceMatrix = createLightSpaceMatrix();
    {
        ScopedGpuPass timer(profiler, "Directional Object Shadow");
        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", state.dirLightSpaceMatrix);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glViewport(0, 0, shadowMap.getWidth(), shadowMap.getHeight());
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.getFBO());
        glClear(GL_DEPTH_BUFFER_BIT);
        sphereDrawer.draw(shadowShader);
        modelDrawer.draw(shadowShader);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    renderCloudOpticalDepth(screenQuad, profiler);
    registry.setTexture(shadowMapHandle, shadowMap.getDepthMapTexture());
}

bool DirectionalShadowPass::cloudShadowNeedsFullUpdate(
    const glm::vec2& snappedOriginLS,
    const glm::vec3& towardSun) const
{
    if (!cloudTextureInitialized)
        return true;
    if (shouldRenderVolumetricClouds(config) != previousCloudEnabled ||
        config.stormHoleSeed != previousHoleSeed ||
        config.stormHoleCount != previousHoleCount)
        return true;
    if (glm::distance(snappedOriginLS, previousCloudOriginLS) > 0.25f)
        return true;
    if (glm::dot(towardSun, previousTowardSun) <
        std::cos(glm::radians(2.0f)))
        return true;
    if (std::abs(config.cloudCoverage - previousWeatherCoverage) > 0.08f ||
        std::abs(config.cloudDensity - previousDensity) > 0.12f ||
        std::abs(config.cloudBaseHeight - previousCloudBaseHeight) > 0.5f ||
        std::abs(config.cloudThickness - previousCloudThickness) > 0.5f ||
        std::abs(config.stormHoleStrength - previousHoleStrength) > 0.10f ||
        std::abs(config.cloudShadowCoverage - previousMapCoverage) > 0.5f)
        return true;
    return false;
}

void DirectionalShadowPass::renderCloudOpticalDepth(
    Screenquad& screenQuad,
    GpuProfiler& profiler)
{
    glm::vec2 snappedOriginLS(0.0f);
    state.cloudShadowMatrix =
        createCloudLightSpaceMatrix(snappedOriginLS);
    state.cloudShadowGlobalTransmission =
        calculateCloudSunTransmission(config);
    const glm::vec3 towardSun = glm::normalize(
        -lightSettings.sunDirection);
    const bool fullUpdate = cloudShadowNeedsFullUpdate(
        snappedOriginLS, towardSun);
    const int slices = std::clamp(config.cloudShadowScanSlices, 2, 8);
    const int rowsPerSlice =
        (CloudShadowResolution + slices - 1) / slices;
    int rowOffset = fullUpdate
        ? 0
        : static_cast<int>(cloudScanSlice) * rowsPerSlice;
    int rowCount = fullUpdate
        ? CloudShadowResolution
        : std::min(rowsPerSlice, CloudShadowResolution - rowOffset);

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glEnable(GL_SCISSOR_TEST);
    glViewport(0, 0, CloudShadowResolution, CloudShadowResolution);
    glScissor(0, rowOffset, CloudShadowResolution, rowCount);

    {
        ScopedGpuPass timer(profiler, "Cloud Tau Integrate");
        renderCloudIntegration(screenQuad, rowOffset, rowCount);
    }
    {
        ScopedGpuPass timer(profiler, "Cloud Tau 3x3 Blur");
        renderCloudBlur(screenQuad, rowOffset, rowCount);
    }
    {
        ScopedGpuPass timer(profiler, "Cloud T Average Mips");
        renderCloudTransmittance(screenQuad, rowOffset, rowCount);
    }

    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDepthMask(depthWriteWasEnabled);
    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    if (blendWasEnabled)
        glEnable(GL_BLEND);

    cloudTextureInitialized = true;
    previousCloudOriginLS = snappedOriginLS;
    previousTowardSun = towardSun;
    previousWeatherCoverage = config.cloudCoverage;
    previousDensity = config.cloudDensity;
    previousCloudBaseHeight = config.cloudBaseHeight;
    previousCloudThickness = config.cloudThickness;
    previousHoleStrength = config.stormHoleStrength;
    previousHoleSeed = config.stormHoleSeed;
    previousHoleCount = config.stormHoleCount;
    previousCloudEnabled = shouldRenderVolumetricClouds(config);
    previousMapCoverage = config.cloudShadowCoverage;
    cloudScanSlice = fullUpdate
        ? 1u % static_cast<unsigned int>(slices)
        : (cloudScanSlice + 1u) % static_cast<unsigned int>(slices);
}

void DirectionalShadowPass::renderCloudIntegration(
    Screenquad& screenQuad,
    int,
    int)
{
    glBindFramebuffer(GL_FRAMEBUFFER, rawCloudFramebuffer);
    cloudOpticalDepthShader.use();
    cloudOpticalDepthShader.setVec2(
        "outputSize",
        glm::vec2(CloudShadowResolution));
    cloudOpticalDepthShader.setMat4(
        "inverseCloudShadowMatrix",
        glm::inverse(state.cloudShadowMatrix));
    cloudOpticalDepthShader.setInt(
        "cloudShadowMarchSteps",
        std::clamp(config.cloudShadowMarchSteps, 4, 8));
    setCloudDensityUniforms(cloudOpticalDepthShader);
    screenQuad.drawTriangle();
}

void DirectionalShadowPass::renderCloudBlur(
    Screenquad& screenQuad,
    int,
    int)
{
    glBindFramebuffer(GL_FRAMEBUFFER, filteredCloudFramebuffer);
    cloudOpticalDepthBlurShader.use();
    cloudOpticalDepthBlurShader.setInt("sourceOpticalDepth", 0);
    cloudOpticalDepthBlurShader.setVec2(
        "inverseTextureSize",
        glm::vec2(1.0f / static_cast<float>(CloudShadowResolution)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rawCloudOpticalDepthTexture);
    screenQuad.drawTriangle();
}

void DirectionalShadowPass::renderCloudTransmittance(
    Screenquad& screenQuad,
    int,
    int)
{
    glBindFramebuffer(GL_FRAMEBUFFER, cloudTransmittanceFramebuffer);
    cloudOpticalDepthToTransmittanceShader.use();
    cloudOpticalDepthToTransmittanceShader.setInt(
        "opticalDepthTexture", 0);
    cloudOpticalDepthToTransmittanceShader.setVec2(
        "inverseTextureSize",
        glm::vec2(1.0f / static_cast<float>(CloudShadowResolution)));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, filteredCloudOpticalDepthTexture);
    screenQuad.drawTriangle();
    glBindTexture(GL_TEXTURE_2D, cloudTransmittanceTextureId);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DirectionalShadowPass::setCloudDensityUniforms(Shader& shader) const
{
    const bool renderClouds = shouldRenderVolumetricClouds(config);
    shader.setVec3("cameraPos", camera.Getposition());
    shader.setVec3(
        "sunDirection", glm::normalize(-lightSettings.sunDirection));
    shader.setFloat("cloudCoverage", config.cloudCoverage);
    shader.setFloat("cloudDensity", renderClouds ? config.cloudDensity : 0.0f);
    shader.setFloat("cloudBaseHeight", config.cloudBaseHeight);
    shader.setFloat("cloudThickness", config.cloudThickness);
    shader.setFloat("cloudScale", config.cloudScale);
    shader.setFloat("cloudDetailScale", config.cloudDetailScale);
    shader.setFloat("cloudType", config.cloudType);
    shader.setFloat("cloudAnvilAmount", config.cloudAnvilAmount);
    shader.setFloat("cloudErosionStrength", config.cloudErosionStrength);
    shader.setFloat("cloudEvolutionTime", config.cloudEvolutionPhase);
    shader.setVec2("cloudWindOffset", config.cloudAnimationOffset);
    shader.setVec2("cloudWindDirection", config.cloudWindDirection);
    shader.setFloat("cloudWindShear", config.cloudWindShear);
    shader.setFloat("cloudLightAbsorption", config.cloudLightAbsorption);
    shader.setFloat("cloudShadowStrength", config.cloudShadowStrength);
    shader.setFloat("stormHoleStrength", config.stormHoleStrength);
    shader.setInt("stormHoleSeed", static_cast<int>(config.stormHoleSeed));
    shader.setVec2("stormHoleAnchor", config.stormHoleAnchor);
    shader.setInt("stormHoleCount", config.stormHoleCount);
    shader.setFloat("stormHoleMinRadius", config.stormHoleMinRadius);
    shader.setFloat("stormHoleMaxRadius", config.stormHoleMaxRadius);
    shader.setFloat("stormHoleSoftness", config.stormHoleSoftness);
}

glm::mat4 DirectionalShadowPass::createCloudLightSpaceMatrix(
    glm::vec2& snappedOriginLS) const
{
    const glm::vec3 towardSun = glm::normalize(
        -lightSettings.sunDirection);
    const glm::vec3 upReference = std::abs(towardSun.y) > 0.99f
        ? glm::vec3(1.0f, 0.0f, 0.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(
        glm::cross(upReference, towardSun));
    const glm::vec3 realUp = glm::normalize(
        glm::cross(towardSun, right));
    const float coverage = std::clamp(
        config.cloudShadowCoverage, 4000.0f, 12000.0f);
    const float texelWorldSize =
        coverage / static_cast<float>(CloudShadowResolution);

    // Anchor the light-space XY footprint to the receiver region. Using the
    // cloud-layer position here shifts ground receivers toward the edge as the
    // layer rises; adding camera.GetFront() also makes that footprint rotate
    // around the camera. Both effects turn a world-space shadow into a
    // camera-relative one. The camera position is a stable approximation of
    // the nearby terrain receiver center and the 8 km default map covers the
    // complete streamed terrain around it.
    const glm::vec3 receiverCenter = camera.Getposition();
    const glm::vec2 originLS(
        glm::dot(receiverCenter, right),
        glm::dot(receiverCenter, realUp));
    snappedOriginLS = glm::floor(originLS / texelWorldSize) * texelWorldSize;

    // Keep the footprint centered on the receivers, but place the depth
    // interval on the point where that receiver ray crosses the middle of the
    // cloud layer. Moving the layer now changes only the integration depth;
    // it cannot push the ground out of the shadow-map XY coverage.
    const float cloudMiddleHeight =
        config.cloudBaseHeight + config.cloudThickness * 0.5f;
    const float sunVertical = std::max(std::abs(towardSun.y), 0.08f);
    const float distanceToCloudMiddle =
        (cloudMiddleHeight - receiverCenter.y) /
        (towardSun.y >= 0.0f ? sunVertical : -sunVertical);
    const glm::vec3 cloudRayCenter =
        receiverCenter + towardSun * distanceToCloudMiddle;
    const glm::vec3 snappedCenter =
        right * snappedOriginLS.x +
        realUp * snappedOriginLS.y +
        towardSun * glm::dot(cloudRayCenter, towardSun);

    // A horizontal layer becomes a longer segment along the light ray when
    // the sun is low. Size the depth interval in ray space so both cloud
    // boundaries remain inside the integration volume at every time of day.
    const float lightDistance =
        std::max(config.cloudThickness, 1.0f) /
        (2.0f * sunVertical) + 250.0f;
    const glm::mat4 view = glm::lookAt(
        snappedCenter + towardSun * lightDistance,
        snappedCenter,
        realUp);
    const float halfCoverage = coverage * 0.5f;
    const glm::mat4 projection = glm::ortho(
        -halfCoverage, halfCoverage,
        -halfCoverage, halfCoverage,
        0.1f, lightDistance * 2.0f);
    return projection * view;
}

glm::mat4 DirectionalShadowPass::createLightSpaceMatrix() const
{
    const bool isTerrain =
        config.sceneSelection == SceneSelection::FujiTerrain;
    const glm::vec3 cameraPosition = camera.Getposition();
    const glm::vec3 sceneCenter = isTerrain
        ? glm::vec3(0.0f, 700.0f, 0.0f)
        : glm::vec3(0.0f, 0.6f, -4.8f);
    const float halfExtent = isTerrain ? 4700.0f : 10.0f;
    const float nearPlane = 0.1f;
    float farPlane = isTerrain ? 5200.0f : 30.0f;
    const glm::vec3 lightDirection = glm::normalize(
        lightSettings.sunDirection);
    float lightDistance = halfExtent;
    if (isTerrain && lightDirection.y < -0.01f)
    {
        const float cloudTop = config.cloudBaseHeight + config.cloudThickness;
        lightDistance = std::max(
            halfExtent,
            (cloudTop - sceneCenter.y) /
                std::max(-lightDirection.y, 0.08f) + 800.0f);
        farPlane = lightDistance + 4500.0f;
    }
    const glm::mat4 projection = glm::ortho(
        -halfExtent, halfExtent,
        -halfExtent, halfExtent,
        nearPlane, farPlane);
    const glm::mat4 view = glm::lookAt(
        sceneCenter - lightDirection * lightDistance,
        sceneCenter,
        glm::vec3(0.0f, 1.0f, 0.0f));
    return projection * view;
}
