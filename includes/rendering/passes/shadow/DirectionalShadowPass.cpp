#include "rendering/passes/shadow/DirectionalShadowPass.h"

DirectionalShadowPass::DirectionalShadowPass( DirectionalShadowMap& shadowMap, Shader& shadowShader, 
    SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
    SceneRenderState& state, LightSettings& lightSettings,
    SceneRenderConfig& config,
    ResourceRegistry& registry, ResourceHandle shadowMapHandle) 
    : shadowMap(shadowMap), shadowShader(shadowShader), 
    sphereDrawer(sphereDrawer), modelDrawer(modelDrawer),
    state(state), lightSettings(lightSettings), config(config),
    registry(registry), shadowMapHandle(shadowMapHandle)
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

        sphereDrawer.draw(shadowShader);
        modelDrawer.draw(shadowShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 将深度纹理注册到资源注册表中
        registry.setTexture(shadowMapHandle, shadowMap.getDepthMapTexture());
    }

glm::mat4 DirectionalShadowPass::createLightSpaceMatrix() const
{
        const bool isTerrain = config.sceneSelection == SceneSelection::FujiTerrain;
        glm::vec3 sceneCenter = isTerrain
            ? glm::vec3(0.0f, 2.0f, -18.0f)
            : glm::vec3(0.0f, 0.6f, -4.8f);
        float halfExtent = isTerrain ? 175.0f : 10.0f;
        float nearPlane = 0.1f;
        float farPlane = isTerrain ? 380.0f : 30.0f;

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
