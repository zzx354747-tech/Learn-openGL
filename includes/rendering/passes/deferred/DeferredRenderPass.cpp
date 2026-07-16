#include "rendering/passes/deferred/DeferredRenderPass.h"

void DeferredRenderPass::render(
    int bfwidth,
    int bfheight,
    Framebuffer& framebuffer,
    Screenquad& screenQuad,
    GpuProfiler& profiler
)
{
    // 1. 几何阶段：写入 GBuffer
    {
        ScopedGpuPass timer(profiler, "Geometry / GBuffer");
        geometryPass.render(bfwidth, bfheight);
    }

    // 2. SSAO 阶段：读取 GBuffer，生成 SSAO 纹理
    if (config.enableSSAO)
    {
        ScopedGpuPass timer(profiler, "SSAO + Blur");
        ssaoCommonPass.render(bfwidth, bfheight);
    }

    // 3. 把 GBuffer 的深度拷贝到 HDR framebuffer
    {
        ScopedGpuPass timer(profiler, "Depth Blit");
        gBuffer.blitDepthTo(framebuffer, bfwidth, bfheight);
    }

    // 4. 延迟光照阶段：读取 GBuffer，输出光照结果到 HDR framebuffer
    {
        ScopedGpuPass timer(profiler, "Deferred Lighting");
        lightingPass.render(framebuffer, screenQuad);
    }
}

DeferredRenderPass::DeferredRenderPass( SceneRenderConfig& config, GeometryPass& geometryPass, SSAOCommonPass& ssaoCommonPass, GBuffer& gBuffer, LightingPass& lightingPass ) : config(config), geometryPass(geometryPass), ssaoCommonPass(ssaoCommonPass), gBuffer(gBuffer), lightingPass(lightingPass)
{
    }
