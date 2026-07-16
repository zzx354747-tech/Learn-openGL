#pragma once

#include <glm/glm.hpp>
#include <cmath>

struct LightSettings
{
    glm::vec3 pointAmbient  = glm::vec3(0.08f);
    glm::vec3 pointDiffuse  = glm::vec3(2.4f);
    glm::vec3 pointSpecular = glm::vec3(1.8f);
    float pointIntensity = 1.0f;
    float pointAmbientIntensity = 1.0f;
    float pointShadowStrength = 0.8f;

    float pointConstant  = 1.0f;
    float pointLinear    = 0.045f;
    float pointQuadratic = 0.0075f;

    glm::vec3 sunDirection = glm::vec3(-0.2f, -1.0f, -0.3f);
    // Direction toward the solar source in the original HDR environment.
    // IBL lookup vectors are rotated from the animated sun back into this
    // source direction, moving the extracted bright region with the sun.
    glm::vec3 environmentSunDirection = glm::vec3(0.2f, 1.0f, 0.3f);
    glm::vec3 sunAmbient   = glm::vec3(0.05f);
    glm::vec3 sunDiffuse   = glm::vec3(0.4f);
    glm::vec3 sunSpecular  = glm::vec3(0.5f);
    float sunIntensity = 1.0f;
    float sunIntensityScale = 1.0f;
    float sunShadowStrength = 0.8f;
    bool sunExtractedFromEnvironment = false;

    glm::vec3 flashAmbient  = glm::vec3(0.05f);
    glm::vec3 flashDiffuse  = glm::vec3(0.8f);
    glm::vec3 flashSpecular = glm::vec3(1.0f);
    float flashIntensity = 1.0f;
    float flashShadowStrength = 0.8f;

    float flashConstant  = 1.0f;
    float flashLinear    = 0.09f;
    float flashQuadratic = 0.032f;

    float flashCutOff      = 12.5f;
    float flashOuterCutOff = 17.5f;

    float flashRightOffset = 0.25f;
    float flashUpOffset = -0.15f;
    float flashForwardOffset = 0.05f;
};

inline glm::mat3 calculateIblSunRotation(const LightSettings& lightSettings)
{
    const glm::vec3 currentValue = -lightSettings.sunDirection;
    const glm::vec3 sourceValue = lightSettings.environmentSunDirection;
    if (glm::dot(currentValue, currentValue) < 1.0e-8f ||
        glm::dot(sourceValue, sourceValue) < 1.0e-8f)
        return glm::mat3(1.0f);

    const glm::vec3 currentSun = glm::normalize(currentValue);
    const glm::vec3 sourceSun = glm::normalize(sourceValue);
    const float cosine = glm::clamp(glm::dot(currentSun, sourceSun),
                                    -1.0f, 1.0f);
    if (cosine > 0.99999f)
        return glm::mat3(1.0f);

    if (cosine < -0.99999f)
    {
        const glm::vec3 reference = std::abs(currentSun.x) < 0.8f
            ? glm::vec3(1.0f, 0.0f, 0.0f)
            : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 axis = glm::normalize(
            glm::cross(currentSun, reference));
        return 2.0f * glm::outerProduct(axis, axis) - glm::mat3(1.0f);
    }

    const glm::vec3 axisTimesSine = glm::cross(currentSun, sourceSun);
    const float sineSquared = glm::dot(axisTimesSine, axisTimesSine);
    const glm::mat3 crossMatrix(
        0.0f, axisTimesSine.z, -axisTimesSine.y,
        -axisTimesSine.z, 0.0f, axisTimesSine.x,
        axisTimesSine.y, -axisTimesSine.x, 0.0f);
    return glm::mat3(1.0f) + crossMatrix +
        (crossMatrix * crossMatrix) * ((1.0f - cosine) / sineSquared);
}
