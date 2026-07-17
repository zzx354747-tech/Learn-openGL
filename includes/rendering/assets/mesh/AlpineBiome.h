#pragma once

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

struct AlpineBiomeParameters
{
    float sunAzimuth = 0.0f;
    float sunHeightShift = 0.006f;
    float noiseHeightShift = 0.008f;
    float grassEnd = 0.28f;
    float rockStart = 0.30f;
    float snowStart = 0.43f;
    float snowEnd = 0.45f;
};

inline float alpineSmoothstep(float a, float b, float x)
{
    if (a == b)
        return x < a ? 0.0f : 1.0f;
    const float t = std::clamp((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// CPU equivalent of src/shader/renderer/common/terrain_biomes.glsl.
// Keep this signature stable: vegetation distribution and a future moisture
// field provider both consume the same normalized TDM classification.
inline glm::vec3 alpineBiomeWeights(float height, float aspect, float noise,
                                    const AlpineBiomeParameters& p)
{
    constexpr float TwoPi = 6.28318530717958647692f;
    const float sunFacing = std::cos(aspect * TwoPi - p.sunAzimuth);
    const float shiftedHeight = height - sunFacing * p.sunHeightShift -
                                (noise - 0.5f) * p.noiseHeightShift;
    const float grassToRock = alpineSmoothstep(p.grassEnd, p.rockStart,
                                                shiftedHeight);
    const float rockToSnow = alpineSmoothstep(p.snowStart, p.snowEnd,
                                               shiftedHeight);
    return glm::vec3(1.0f - grassToRock,
                     grassToRock * (1.0f - rockToSnow),
                     grassToRock * rockToSnow);
}
