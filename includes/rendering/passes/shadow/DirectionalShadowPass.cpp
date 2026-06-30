#include "rendering/passes/shadow/DirectionalShadowPass.h"

DirectionalShadowPass::DirectionalShadowPass( DirectionalShadowMap& shadowMap, Shader& shadowShader, SceneDrawer& drawer, SceneRenderState& state, LightSettings& lightSettings, ResourceRegistry& registry, ResourceHandle shadowMapHandle) : shadowMap(shadowMap), shadowShader(shadowShader), drawer(drawer), state(state), lightSettings(lightSettings), registry(registry), shadowMapHandle(shadowMapHandle)
{
    }

void DirectionalShadowPass::render()
{
        state.dirLightSpaceMatrix = createLightSpaceMatrix();

        shadowShader.use();
        shadowShader.setMat4("lightSpaceMatrix", state.dirLightSpaceMatrix);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glViewport(0, 0, shadowMap.getWidth(), shadowMap.getHeight());
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.getFBO());
        glClear(GL_DEPTH_BUFFER_BIT);

        drawer.drawScene(shadowShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 将深度纹理注册到资源注册表中
        registry.setTexture(shadowMapHandle, shadowMap.getDepthMapTexture());
    }

glm::mat4 DirectionalShadowPass::createLightSpaceMatrix() const
{
        glm::vec3 sceneCenter = drawer.getActiveSceneWorldCenter();
        glm::vec3 sceneSize = drawer.getActiveSceneWorldSize();
        float sceneExtent = glm::max(sceneSize.x, glm::max(sceneSize.y, sceneSize.z));
        float halfExtent = glm::max(sceneExtent * 0.75f, 10.0f);
        float nearPlane = 0.1f;
        float farPlane = glm::max(sceneExtent * 2.5f, 30.0f);

        glm::mat4 lightProjection = glm::ortho(
            -halfExtent,
            halfExtent,
            -halfExtent,
            halfExtent,
            nearPlane,
            farPlane);

        glm::vec3 lightDirection = glm::normalize(lightSettings.sunDirection);
        glm::mat4 lightView = glm::lookAt(
            sceneCenter - lightDirection * halfExtent,
            sceneCenter,
            glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProjection * lightView;
    }
