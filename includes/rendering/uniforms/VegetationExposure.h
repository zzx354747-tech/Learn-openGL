#pragma once

#include <cmath>

#include <glm/glm.hpp>

#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/render/SceneRenderTypes.h"

inline float calculateVegetationExposureCoefficient(
    const SceneRenderConfig& config)
{
    const float userCoefficient = glm::clamp(
        config.vegetationExposureCoefficient, 0.25f, 2.0f);
    // This is the same global cloud-shadow transmission used by the sun and
    // directional shadow setup. Weather transitions therefore drive the
    // vegetation coefficient continuously instead of switching at a preset
    // boundary.
    const float cloudShadowTransmission = calculateCloudSunTransmission(config);
    return glm::clamp(
        userCoefficient * cloudShadowTransmission, 0.04f, 2.0f);
}

// Vegetation does not receive direct lighting. The target is a compressed
// response to the original light energy, not a direct radiance multiplier.
// The lighting pass may add screen-space GI on top of this base colour.
inline float calculateVegetationExposureTarget(
    const SceneRenderConfig& config,
    const LightSettings& lights)
{
    constexpr float Pi = 3.14159265359f;
    constexpr glm::vec3 Luma(0.2126f, 0.7152f, 0.0722f);
    const auto luminance = [&](const glm::vec3& color)
    {
        return glm::dot(glm::max(color, glm::vec3(0.0f)), Luma);
    };

    float sourceBrightness = 0.0f;
    if (config.enableDirectionalLight)
    {
        const glm::vec3 sunRadiance = lights.sunDiffuse *
            lights.sunIntensity * lights.sunIntensityScale *
            glm::clamp(config.daylightFactor, 0.0f, 1.0f);
        sourceBrightness += luminance(sunRadiance) * 0.5f / Pi *
            (config.enablePBR ? 1.0f :
             glm::max(config.phongDiffuseStrength, 0.0f)) *
            calculateCloudSunTransmission(config);
    }

    if (config.enablePointLight)
    {
        constexpr float ReferenceDistance = 8.0f;
        const float attenuation = 1.0f / (
            lights.pointConstant + lights.pointLinear * ReferenceDistance +
            lights.pointQuadratic * ReferenceDistance * ReferenceDistance);
        sourceBrightness += luminance(lights.pointDiffuse *
            lights.pointIntensity) * attenuation * 0.5f / Pi;
    }

    if (config.enableFlashlight)
    {
        constexpr float ReferenceDistance = 6.0f;
        const float attenuation = 1.0f / (
            lights.flashConstant + lights.flashLinear * ReferenceDistance +
            lights.flashQuadratic * ReferenceDistance * ReferenceDistance);
        sourceBrightness += luminance(lights.flashDiffuse *
            lights.flashIntensity) * attenuation * 0.5f / Pi;
    }

    if (config.enableIBL)
    {
        sourceBrightness += luminance(config.iblAmbientTint) *
            config.iblAmbientStrength *
            (config.enablePBR ? 1.0f :
             glm::max(config.phongIBLDiffuseStrength, 0.0f)) *
            calculateCloudAmbientTransmission(config);
    }
    else
    {
        sourceBrightness += luminance(config.fixedAmbientColor) *
            config.fixedAmbientStrength *
            calculateCloudAmbientTransmission(config);
    }

    // Map source energy into a bounded exposure range. This remains monotonic
    // (brighter original light -> higher vegetation exposure), but saturates
    // smoothly instead of passing HDR sun values straight into the albedo.
    const float normalizedBrightness = sourceBrightness /
        (sourceBrightness + 1.5f);
    const float displayExposure = glm::mix(
        0.32f, 0.95f,
        glm::clamp(normalizedBrightness, 0.0f, 1.0f));

    // ScreenPass applies config.exposure to the whole HDR buffer afterward.
    // Compensate that stage here so a bright preset with exposure 0.5 does
    // not make otherwise-correct unlit vegetation look unnecessarily dark.
    const float globalExposure = glm::max(config.exposure, 0.25f);
    const float coefficient = calculateVegetationExposureCoefficient(config);
    return glm::clamp(
        displayExposure / globalExposure * coefficient, 0.04f, 1.8f);
}

inline void updateVegetationExposure(
    SceneRenderState& state,
    float targetExposure,
    float deltaTime)
{
    targetExposure = glm::clamp(targetExposure, 0.04f, 1.8f);
    if (!state.vegetationExposureInitialized)
    {
        state.vegetationExposure = targetExposure;
        state.vegetationExposureInitialized = true;
        return;
    }

    const float dt = glm::clamp(deltaTime, 0.0f, 0.10f);
    // Adapt faster toward a darker target to remove overexposure quickly,
    // while brightening more gently to avoid visible pumping.
    const float response = targetExposure < state.vegetationExposure
        ? 6.0f : 2.5f;
    const float alpha = 1.0f - std::exp(-response * dt);
    state.vegetationExposure = glm::mix(
        state.vegetationExposure, targetExposure, alpha);
}
