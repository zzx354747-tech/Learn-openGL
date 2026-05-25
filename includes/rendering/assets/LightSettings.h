#pragma once

#include <glm/glm.hpp>

struct LightSettings
{
    glm::vec3 pointAmbient  = glm::vec3(0.05f);
    glm::vec3 pointDiffuse  = glm::vec3(0.8f);
    glm::vec3 pointSpecular = glm::vec3(1.0f);

    float pointConstant  = 1.0f;
    float pointLinear    = 0.09f;
    float pointQuadratic = 0.032f;

    glm::vec3 sunDirection = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 sunAmbient   = glm::vec3(0.05f);
    glm::vec3 sunDiffuse   = glm::vec3(0.4f);
    glm::vec3 sunSpecular  = glm::vec3(0.5f);

    glm::vec3 flashAmbient  = glm::vec3(0.05f);
    glm::vec3 flashDiffuse  = glm::vec3(0.8f);
    glm::vec3 flashSpecular = glm::vec3(1.0f);

    float flashConstant  = 1.0f;
    float flashLinear    = 0.09f;
    float flashQuadratic = 0.032f;

    float flashCutOff      = 12.5f;
    float flashOuterCutOff = 17.5f;

    float flashRightOffset = 0.25f;
    float flashUpOffset = -0.15f;
    float flashForwardOffset = 0.05f;
};
