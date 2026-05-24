#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/Shader.h"
#include "rendering/postprocess/PointShadowMap.h"
#include "rendering/core/SceneDrawer.h"

// 把场景从点光源的位置，从六个方向渲染到depth cube map上
class PointShadowPass
{
public: 
    PointShadowPass(PointShadowMap& shadowMap, 
        Shader& shadowShader, 
        SceneDrawer& sceneDrawer)
        :pointShadowMap(shadowMap), 
        shadowShader(shadowShader), 
        sceneDrawer(sceneDrawer)
    {}

    void render(const glm::vec3& lightpos);

private:
    PointShadowMap& pointShadowMap;
    Shader& shadowShader;
    SceneDrawer& sceneDrawer;

    std::vector<glm::mat4> createShadowTransforms(const glm::vec3& lightPos, 
        float nearPlane, 
        float farPlane);
};