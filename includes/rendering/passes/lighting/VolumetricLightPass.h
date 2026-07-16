#pragma once

#include "rendering/assets/light/LightSettings.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/debug/GpuProfiler.h"
#include "rendering/passes/environment/CloudAccelerationPass.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "scene/Camera.h"

class VolumetricLightPass
{
public:
    VolumetricLightPass(
        Camera& camera,
        ShadowResources& shadowResources,
        SceneRenderResources& resources,
        SceneRenderConfig& config,
        SceneRenderState& state,
        LightSettings& lightSettings,
        int width,
        int height);

    void render(
        int width,
        int height,
        Framebuffer& destination,
        Screenquad& screenQuad,
        const CloudAccelerationPass& cloudAccelerationPass,
        GpuProfiler& profiler);

    void resize(int width, int height);

private:
    void ensureTargetSize(int width, int height);
    void copySceneDepth(int width, int height, Framebuffer& source);
    void rayMarch(
        int width,
        int height,
        Screenquad& screenQuad,
        const CloudAccelerationPass& cloudAccelerationPass);
    void bilateralComposite(
        int width,
        int height,
        Framebuffer& destination,
        Screenquad& screenQuad);

    Camera& camera_;
    ShadowResources& shadowResources_;
    SceneRenderResources& resources_;
    SceneRenderConfig& config_;
    SceneRenderState& state_;
    LightSettings& lightSettings_;
    Framebuffer halfResolutionTarget_;
    Framebuffer depthCopyTarget_;
    int targetWidth_ = 1;
    int targetHeight_ = 1;
    int fullWidth_ = 1;
    int fullHeight_ = 1;
    unsigned int frameIndex_ = 0;
};
