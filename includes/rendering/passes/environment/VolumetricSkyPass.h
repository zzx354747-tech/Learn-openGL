#pragma once

#include "rendering/assets/light/LightSettings.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/debug/GpuProfiler.h"
#include "rendering/passes/environment/CloudAccelerationPass.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "scene/Camera.h"

class VolumetricSkyPass
{
public:
    VolumetricSkyPass(
        Camera& camera,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        LightSettings& lightSettings,
        int width,
        int height);

    void render(
        int width,
        int height,
        Framebuffer& destination,
        Screenquad& screenQuad,
        GpuProfiler& profiler);

    void updateCloudAcceleration(
        Screenquad& screenQuad,
        GpuProfiler& profiler);

    void resize(int width, int height);

    const CloudAccelerationPass& cloudAccelerationPass() const
    {
        return cloudAccelerationPass_;
    }

private:
    void ensureRenderTargetSize(int width, int height);
    void composite(
        int width,
        int height,
        Framebuffer& destination,
        Screenquad& screenQuad);

    Camera& camera_;
    SceneRenderResources& resources_;
    SceneRenderConfig& config_;
    LightSettings& lightSettings_;
    CloudAccelerationPass cloudAccelerationPass_;
    Framebuffer lowResolutionTarget_;
    int targetWidth_ = 1;
    int targetHeight_ = 1;
};
