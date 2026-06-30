#include "rendering/passes/debug/ShadowDebugPass.h"

ShadowDebugPass::ShadowDebugPass( SceneRenderResources& resources, ShadowResources& shadowResources ) : resources(resources), shadowResources(shadowResources)
{
    }

void ShadowDebugPass::render(int bfwidth, int bfheight, Screenquad& screenQuad)
{
        if (!resources.shadowDebugShader || !shadowResources.shadowMap)
            return;

        glViewport(0, 0, bfwidth, bfheight);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Debug Pass 是全屏 quad，不需要深度测试
        glDisable(GL_DEPTH_TEST);

        resources.shadowDebugShader->use();
        resources.shadowDebugShader->setInt("depthMap", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(
            GL_TEXTURE_2D,
            shadowResources.shadowMap->getDepthMapTexture()
        );

        screenQuad.draw();
    }
