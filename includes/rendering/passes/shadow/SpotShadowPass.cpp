#include "rendering/passes/shadow/SpotShadowPass.h"

SpotShadowPass::SpotShadowPass( SpotShadowMap& spotShadowMap, Shader& spotShadowShader,
     SphereDrawer& sphereDrawer, ModelDrawer& modelDrawer,
     Camera& camera, SceneRenderState& state,
      LightSettings& lightSettings, ResourceRegistry& registry,
       ResourceHandle spotShadowMapHandle) 
       : spotShadowMap(spotShadowMap), spotShadowShader(spotShadowShader), 
       sphereDrawer(sphereDrawer), modelDrawer(modelDrawer),
       camera(camera), state(state), 
       lightSettings(lightSettings), registry(registry), 
       spotShadowMapHandle(spotShadowMapHandle)
{
    }

void SpotShadowPass::render()
{
        lightSpaceMatrix = createLightSpaceMatrix();
        state.spotLightSpaceMatrix = lightSpaceMatrix;

        spotShadowShader.use();
        spotShadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        glViewport(
            0,
            0,
            spotShadowMap.getWidth(),
            spotShadowMap.getHeight()
        );

        glBindFramebuffer(GL_FRAMEBUFFER, spotShadowMap.getFBO());
        glClear(GL_DEPTH_BUFFER_BIT);

        sphereDrawer.draw(spotShadowShader);
        modelDrawer.draw(spotShadowShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        registry.setTexture(
            spotShadowMapHandle,
            spotShadowMap.getDepthMap()
        );
    }

glm::mat4 SpotShadowPass::createLightSpaceMatrix()
{
        glm::mat4 lightProjection = glm::perspective(
            glm::radians(45.0f),
            1.0f,
            spotShadowMap.getNearPlane(),
            spotShadowMap.getFarPlane()
        );

        glm::vec3 lightPos = getFlashLightPosition();
        glm::vec3 lightDir = camera.GetFront();

        glm::mat4 lightView = glm::lookAt(
            lightPos,
            lightPos + lightDir,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        return lightProjection * lightView;
    }

glm::vec3 SpotShadowPass::getFlashLightPosition() const
{
        glm::vec3 front = camera.GetFront();
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, front));

        return camera.Getposition()
            + right * lightSettings.flashRightOffset
            + up * lightSettings.flashUpOffset
            + front * lightSettings.flashForwardOffset;
    }
