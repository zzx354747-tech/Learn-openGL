#pragma once

#include <glm/glm.hpp>

#include "core/Shader.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/debug/GpuProfiler.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "scene/Camera.h"

class CloudAccelerationPass
{
public:
    CloudAccelerationPass(
        Camera& camera,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        LightSettings& lightSettings);

    void update(Screenquad& screenQuad, GpuProfiler& profiler);
    void bindForCloudShader(Shader& shader, int textureUnit = 3) const;

private:
    void resizeIfNeeded();
    void updateCachePlacement();
    void setGenerationUniforms(
        Shader& shader,
        const glm::vec2& origin,
        float worldSize,
        int resolution) const;
    void renderRegion(
        Screenquad& screenQuad,
        Framebuffer& target,
        const glm::vec2& origin,
        float worldSize,
        int resolution,
        int tileIndex,
        int tilesPerAxis,
        bool fullUpdate);
    void generateMipmaps(const Framebuffer& target) const;
    void publishBindings();

    Camera& camera_;
    SceneRenderResources& resources_;
    SceneRenderConfig& config_;
    LightSettings& lightSettings_;
    Framebuffer cacheTarget_;
    Framebuffer sunLocalCacheTarget_;
    glm::vec2 cacheOrigin_ = glm::vec2(0.0f);
    glm::vec2 sunLocalCacheOrigin_ = glm::vec2(0.0f);
    float cacheWorldSize_ = 1.0f;
    float sunLocalCacheWorldSize_ = 28000.0f;
    int cacheResolution_ = 256;
    int sunLocalCacheResolution_ = 512;
    unsigned int frameIndex_ = 0;
    int updateTile_ = 0;
    int sunLocalUpdateTile_ = 0;
    bool cacheValid_ = false;
    bool sunLocalCacheValid_ = false;
    bool placementChanged_ = true;
    bool sunLocalPlacementChanged_ = true;
};
