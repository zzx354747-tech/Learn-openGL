#include "rendering/passes/environment/CloudAccelerationPass.h"

#include <algorithm>
#include <cmath>

CloudAccelerationPass::CloudAccelerationPass(
    Camera& camera,
    SceneRenderResources& resources,
    SceneRenderConfig& config,
    LightSettings& lightSettings)
    : camera_(camera)
    , resources_(resources)
    , config_(config)
    , lightSettings_(lightSettings)
    , cacheTarget_(256, 256)
    , sunLocalCacheTarget_(512, 512)
{}

void CloudAccelerationPass::update(
    Screenquad& screenQuad,
    GpuProfiler& profiler)
{
    const bool anyAccelerationEnabled =
        config_.enableCloudOccupancySkipping || config_.enableCloudLightCache;
    if (!config_.enableCloudAcceleration || !anyAccelerationEnabled ||
        !resources_.shaderLibrary)
    {
        cacheValid_ = false;
        sunLocalCacheValid_ = false;
        publishBindings();
        return;
    }

    resizeIfNeeded();
    updateCachePlacement();

    const int updateInterval = std::max(config_.cloudCacheUpdateInterval, 1);
    const bool scheduledUpdate = (frameIndex_ %
        static_cast<unsigned int>(updateInterval)) == 0u;
    ++frameIndex_;

    const bool globalFullUpdate = placementChanged_ || !cacheValid_;
    if (globalFullUpdate || scheduledUpdate)
    {
        ScopedGpuPass timer(profiler, "Cloud Global Cache");
        renderRegion(
            screenQuad,
            cacheTarget_,
            cacheOrigin_,
            cacheWorldSize_,
            cacheResolution_,
            updateTile_,
            2,
            globalFullUpdate);
        generateMipmaps(cacheTarget_);
        cacheValid_ = true;
        placementChanged_ = false;
        updateTile_ = globalFullUpdate ? 0 : (updateTile_ + 1) % 4;
    }

    if (!config_.enableSunLocalCloudCache)
    {
        sunLocalCacheValid_ = false;
        publishBindings();
        return;
    }

    const bool localFullUpdate =
        sunLocalPlacementChanged_ || !sunLocalCacheValid_;
    if (localFullUpdate || scheduledUpdate)
    {
        const int tilesPerAxis = std::clamp(
            config_.sunLocalCloudCacheTilesPerAxis, 2, 8);
        const int tileCount = tilesPerAxis * tilesPerAxis;
        ScopedGpuPass timer(profiler, "Cloud Sun-local Cache");
        renderRegion(
            screenQuad,
            sunLocalCacheTarget_,
            sunLocalCacheOrigin_,
            sunLocalCacheWorldSize_,
            sunLocalCacheResolution_,
            sunLocalUpdateTile_,
            tilesPerAxis,
            localFullUpdate);
        generateMipmaps(sunLocalCacheTarget_);
        sunLocalCacheValid_ = true;
        sunLocalPlacementChanged_ = false;
        sunLocalUpdateTile_ = localFullUpdate
            ? 0
            : (sunLocalUpdateTile_ + 1) % tileCount;
    }
    publishBindings();
}

void CloudAccelerationPass::bindForCloudShader(
    Shader& shader,
    int textureUnit) const
{
    const bool enabled = shouldRenderVolumetricClouds(config_) &&
        config_.enableCloudAcceleration && cacheValid_ &&
        (config_.enableCloudOccupancySkipping || config_.enableCloudLightCache);
    const bool localEnabled = enabled && config_.enableSunLocalCloudCache &&
        sunLocalCacheValid_;

    shader.use();
    shader.setBool("hasCloudAccelerationMap", enabled);
    shader.setBool("hasCloudSunLocalMap", localEnabled);
    shader.setBool("enableCloudOccupancySkipping",
                   config_.enableCloudOccupancySkipping);
    shader.setBool("enableCloudLightCache",
                   config_.enableCloudLightCache);
    shader.setVec2("cloudCacheOrigin", cacheOrigin_);
    shader.setFloat("cloudCacheWorldSize", cacheWorldSize_);
    shader.setVec2("cloudSunLocalOrigin", sunLocalCacheOrigin_);
    shader.setFloat("cloudSunLocalWorldSize", sunLocalCacheWorldSize_);
    shader.setFloat("cloudEmptySkipMultiplier",
                    config_.cloudEmptySkipMultiplier);
    shader.setFloat("cloudOccupancyThreshold",
                    config_.cloudOccupancyThreshold);

    shader.setInt("cloudAccelerationMap", textureUnit);
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D,
                  enabled ? cacheTarget_.getTextureID(0) : 0);

    shader.setInt("cloudSunLocalMap", textureUnit + 1);
    glActiveTexture(GL_TEXTURE0 + textureUnit + 1);
    glBindTexture(GL_TEXTURE_2D,
                  localEnabled ? sunLocalCacheTarget_.getTextureID(0) : 0);
}

void CloudAccelerationPass::resizeIfNeeded()
{
    const int requestedResolution = std::clamp(
        config_.cloudCacheResolution, 64, 512);
    if (requestedResolution != cacheResolution_)
    {
        cacheResolution_ = requestedResolution;
        cacheTarget_.resize(cacheResolution_, cacheResolution_);
        cacheValid_ = false;
        placementChanged_ = true;
    }

    const int requestedLocalResolution = std::clamp(
        config_.sunLocalCloudCacheResolution, 256, 1024);
    if (requestedLocalResolution != sunLocalCacheResolution_)
    {
        sunLocalCacheResolution_ = requestedLocalResolution;
        sunLocalCacheTarget_.resize(
            sunLocalCacheResolution_, sunLocalCacheResolution_);
        sunLocalCacheValid_ = false;
        sunLocalPlacementChanged_ = true;
    }
}

void CloudAccelerationPass::updateCachePlacement()
{
    const float requestedSize = std::max(
        config_.cloudCacheWorldSize,
        config_.cloudMaxDistance * 2.10f);
    const float texelWorldSize = requestedSize /
        static_cast<float>(std::max(cacheResolution_, 1));
    const float snapDistance = texelWorldSize * 8.0f;
    const glm::vec3 cameraPosition = camera_.Getposition();
    const glm::vec2 snappedCenter(
        std::floor(cameraPosition.x / snapDistance) * snapDistance,
        std::floor(cameraPosition.z / snapDistance) * snapDistance);
    const glm::vec2 requestedOrigin =
        snappedCenter - glm::vec2(requestedSize * 0.5f);

    if (std::abs(requestedSize - cacheWorldSize_) > 0.5f ||
        glm::distance(requestedOrigin, cacheOrigin_) > 0.5f)
    {
        cacheWorldSize_ = requestedSize;
        cacheOrigin_ = requestedOrigin;
        placementChanged_ = true;
    }

    const float requestedLocalSize = std::clamp(
        config_.sunLocalCloudCacheWorldSize, 20000.0f, 32000.0f);
    const glm::vec3 towardSun = glm::normalize(-lightSettings_.sunDirection);
    const float cloudMiddle =
        config_.cloudBaseHeight + config_.cloudThickness * 0.5f;
    const float distanceToCloud = std::max(
        (cloudMiddle - cameraPosition.y) / std::max(towardSun.y, 0.08f),
        0.0f);
    const glm::vec2 sunCloudCenter = glm::vec2(
        cameraPosition.x, cameraPosition.z) +
        glm::vec2(towardSun.x, towardSun.z) * distanceToCloud;
    // During storm formation, dedicate the high-resolution cache to the fixed
    // weather-system anchor. Only cache placement changes here; the aperture
    // itself never contains a camera-relative coordinate.
    const float fixedAnchorWeight = glm::smoothstep(
        0.05f, 0.55f, glm::clamp(config_.stormHoleStrength, 0.0f, 1.0f));
    const glm::vec2 localCacheCenter = glm::mix(
        sunCloudCenter, config_.stormHoleAnchor, fixedAnchorWeight);
    const float localTexelWorldSize = requestedLocalSize /
        static_cast<float>(std::max(sunLocalCacheResolution_, 1));
    const float localSnapDistance = localTexelWorldSize * 8.0f;
    const glm::vec2 snappedSunCenter(
        std::floor(localCacheCenter.x / localSnapDistance) * localSnapDistance,
        std::floor(localCacheCenter.y / localSnapDistance) * localSnapDistance);
    const glm::vec2 requestedLocalOrigin =
        snappedSunCenter - glm::vec2(requestedLocalSize * 0.5f);

    if (std::abs(requestedLocalSize - sunLocalCacheWorldSize_) > 0.5f ||
        glm::distance(requestedLocalOrigin, sunLocalCacheOrigin_) > 0.5f)
    {
        sunLocalCacheWorldSize_ = requestedLocalSize;
        sunLocalCacheOrigin_ = requestedLocalOrigin;
        sunLocalPlacementChanged_ = true;
    }
}

void CloudAccelerationPass::setGenerationUniforms(
    Shader& shader,
    const glm::vec2& origin,
    float worldSize,
    int resolution) const
{
    shader.setVec2("cloudCacheOrigin", origin);
    shader.setFloat("cloudCacheWorldSize", worldSize);
    shader.setFloat("cloudCacheCellSize",
                    worldSize / static_cast<float>(resolution));
    shader.setVec3("cameraPos", camera_.Getposition());
    shader.setVec3("sunDirection", glm::normalize(-lightSettings_.sunDirection));
    shader.setFloat("cloudCoverage", config_.cloudCoverage);
    shader.setFloat("cloudDensity", config_.cloudDensity);
    shader.setFloat("cloudBaseHeight", config_.cloudBaseHeight);
    shader.setFloat("cloudThickness", config_.cloudThickness);
    shader.setFloat("cloudScale", config_.cloudScale);
    shader.setFloat("cloudDetailScale", config_.cloudDetailScale);
    shader.setFloat("cloudType", config_.cloudType);
    shader.setFloat("cloudAnvilAmount", config_.cloudAnvilAmount);
    shader.setFloat("cloudErosionStrength", config_.cloudErosionStrength);
    shader.setFloat("cloudEvolutionTime", config_.cloudEvolutionPhase);
    shader.setVec2("cloudWindOffset", config_.cloudAnimationOffset);
    shader.setVec2("cloudWindDirection", config_.cloudWindDirection);
    shader.setFloat("cloudWindShear", config_.cloudWindShear);
    shader.setFloat("cloudLightAbsorption", config_.cloudLightAbsorption);
    shader.setInt("cloudCacheLightSteps", config_.cloudCacheLightSteps);
    shader.setFloat("stormHoleStrength", config_.stormHoleStrength);
    shader.setInt("stormHoleSeed", static_cast<int>(config_.stormHoleSeed));
    shader.setVec2("stormHoleAnchor", config_.stormHoleAnchor);
    shader.setInt("stormHoleCount", config_.stormHoleCount);
    shader.setFloat("stormHoleMinRadius", config_.stormHoleMinRadius);
    shader.setFloat("stormHoleMaxRadius", config_.stormHoleMaxRadius);
    shader.setFloat("stormHoleSoftness", config_.stormHoleSoftness);
}

void CloudAccelerationPass::renderRegion(
    Screenquad& screenQuad,
    Framebuffer& target,
    const glm::vec2& origin,
    float worldSize,
    int resolution,
    int tileIndex,
    int tilesPerAxis,
    bool fullUpdate)
{
    Shader& shader = resources_.shaderLibrary->cloudAcceleration;
    target.bind();
    glViewport(0, 0, resolution, resolution);

    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint previousScissorBox[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, previousScissorBox);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    if (fullUpdate)
    {
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else
    {
        const int tiles = std::max(tilesPerAxis, 1);
        const int tileX = tileIndex % tiles;
        const int tileY = tileIndex / tiles;
        const int x0 = resolution * tileX / tiles;
        const int y0 = resolution * tileY / tiles;
        const int x1 = resolution * (tileX + 1) / tiles;
        const int y1 = resolution * (tileY + 1) / tiles;
        glEnable(GL_SCISSOR_TEST);
        glScissor(x0, y0, x1 - x0, y1 - y0);
    }

    shader.use();
    setGenerationUniforms(shader, origin, worldSize, resolution);
    screenQuad.draw();

    glScissor(previousScissorBox[0], previousScissorBox[1],
              previousScissorBox[2], previousScissorBox[3]);
    if (scissorWasEnabled)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
    if (depthTestWasEnabled)
        glEnable(GL_DEPTH_TEST);
    if (blendWasEnabled)
        glEnable(GL_BLEND);
    target.unbind();
}

void CloudAccelerationPass::generateMipmaps(
    const Framebuffer& target) const
{
    glBindTexture(GL_TEXTURE_2D, target.getTextureID(0));
    glTexParameteri(
        GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CloudAccelerationPass::publishBindings()
{
    const bool enabled = shouldRenderVolumetricClouds(config_) &&
        config_.enableCloudAcceleration && cacheValid_;
    resources_.cloudAccelerationValid = enabled;
    resources_.cloudSunLocalValid = enabled &&
        config_.enableSunLocalCloudCache && sunLocalCacheValid_;
    resources_.cloudAccelerationTexture = enabled
        ? cacheTarget_.getTextureID(0) : 0;
    resources_.cloudSunLocalTexture = resources_.cloudSunLocalValid
        ? sunLocalCacheTarget_.getTextureID(0) : 0;
    resources_.cloudCacheOrigin = cacheOrigin_;
    resources_.cloudSunLocalOrigin = sunLocalCacheOrigin_;
    resources_.cloudCacheWorldSize = cacheWorldSize_;
    resources_.cloudSunLocalWorldSize = sunLocalCacheWorldSize_;
}
