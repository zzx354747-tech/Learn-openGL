#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "rendering/resources/shadow/PointShadowMap.h"
#include "rendering/core/SceneDrawer.h"
#include "rendering/core/ResourceRegistry.h"

// 把场景从点光源的位置，从六个方向渲染到depth cube map上
class PointShadowPass
{
public: 
    PointShadowPass(PointShadowMap& shadowMap, Shader& shadowShader, SceneDrawer& sceneDrawer, ResourceRegistry& registry, ResourceHandle depthCubeMapHandle);

    void render(const glm::vec3& lightpos);

private:
    PointShadowMap& pointShadowMap;
    Shader& shadowShader;
    SceneDrawer& sceneDrawer;
    ResourceRegistry& registry;
    ResourceHandle depthCubeMapHandle;

    std::vector<glm::mat4> createShadowTransforms(const glm::vec3& lightPos, 
        float nearPlane, 
        float farPlane);
};
