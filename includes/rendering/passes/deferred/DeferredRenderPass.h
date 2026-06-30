#pragma once

#include "rendering/resources/render/SceneRenderTypes.h"

#include "rendering/passes/geometry/GeometryPass.h"
#include "rendering/passes/lighting/LightingPass.h"
#include "rendering/passes/ssao/SSAOCommonPass.h"

#include "rendering/resources/framebuffer/Gbuffer.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"

class DeferredRenderPass
{
public:
    DeferredRenderPass( SceneRenderConfig& config, GeometryPass& geometryPass, SSAOCommonPass& ssaoCommonPass, GBuffer& gBuffer, LightingPass& lightingPass );

    void render(
        int bfwidth,
        int bfheight,
        Framebuffer& framebuffer,
        Screenquad& screenQuad
    );

private:
    SceneRenderConfig& config;

    GeometryPass& geometryPass;
    SSAOCommonPass& ssaoCommonPass;
    GBuffer& gBuffer;
    LightingPass& lightingPass;
};
