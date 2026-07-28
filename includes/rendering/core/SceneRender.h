#pragma once

#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/passes/blur/BrightPrefilterPass.h"
#include "rendering/passes/debug/ShadowDebugPass.h"
#include "rendering/passes/deferred/DeferredRenderPass.h"
#include "rendering/passes/forward/ForwardHDRPass.h"
#include "rendering/passes/forward/ForwardOverlayPass.h"
#include "rendering/passes/geometry/GeometryPass.h"
#include "rendering/passes/lighting/LightingPass.h"
#include "rendering/passes/ssao/SSAOCommonPass.h"
#include "rendering/passes/shadow/DirectionalShadowPass.h"
#include "rendering/passes/shadow/PointShadowPass.h"
#include "rendering/passes/shadow/SpotShadowPass.h"
#include "rendering/passes/blur/BlurPass.h"
#include "rendering/passes/screen/ScreenPass.h"
#include "rendering/resources/framebuffer/Gbuffer.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/core/ModelDrawer.h"

class SceneRender
{
public:
    SceneRender( Camera& camera, ShadowResources& shadowResources,
        SceneRenderResources& resources, SceneRenderConfig& config,
        SceneRenderState& state, LightSettings& lightSettings,
        DirectionalShadowPass& directionalShadowPass, BrightPrefilterPass& brightPrefilterPass,
        PointShadowPass& pointShadowPass, SpotShadowPass& spotShadowPass,
        GeometryPass& geometryPass, LightingPass& lightingPass,
        GBuffer& gBuffer, SSAOCommonPass& ssaoCommonPass,
        SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer);

    void render( int bfwidth, int bfheight,
        Shader& screenShader, Screenquad& screenQuad,
        Framebuffer& framebuffer);

private:
    SceneRenderConfig& config;
    SceneRenderResources& resources;
    SceneRenderState& state;
    BrightPrefilterPass& brightPrefilterPass;
    DirectionalShadowPass& directionalShadowPass;
    PointShadowPass& pointShadowPass;
    SpotShadowPass& spotShadowPass;
    ShadowDebugPass shadowDebugPass;
    DeferredRenderPass deferredRenderPass;
    ForwardHDRPass forwardHDRPass;
    ForwardOverlayPass forwardOverlayPass;
    BlurPass blurPass;
    ScreenPass screenPass;
};
