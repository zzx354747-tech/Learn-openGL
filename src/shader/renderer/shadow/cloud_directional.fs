#version 330 core

in vec2 TexCoords;
layout (location = 0) out vec4 CloudShadowData;

uniform mat4 lightSpaceMatrix;
uniform mat4 inverseLightSpaceMatrix;
uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform float cloudCoverage;
uniform float cloudDensity;
uniform float cloudBaseHeight;
uniform float cloudThickness;
uniform float cloudScale;
uniform float cloudDetailScale;
uniform float cloudType;
uniform float cloudAnvilAmount;
uniform float cloudErosionStrength;
uniform float cloudEvolutionTime;
uniform vec2 cloudWindOffset;
uniform vec2 cloudWindDirection;
uniform float cloudWindShear;
uniform float cloudLightAbsorption;
uniform float cloudShadowStrength;
uniform float stormHoleStrength;
uniform int stormHoleSeed;
uniform vec2 stormHoleAnchor;
uniform int stormHoleCount;
uniform float stormHoleMinRadius;
uniform float stormHoleMaxRadius;
uniform float stormHoleSoftness;

#include "../common/cloud_density.glsl"

const int CLOUD_SHADOW_STEPS = 24;

float shadowJitter(vec2 pixel)
{
    vec3 p3 = fract(vec3(pixel.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main()
{
    vec2 ndc = TexCoords * 2.0 - 1.0;
    vec4 nearPoint = inverseLightSpaceMatrix * vec4(ndc, -1.0, 1.0);
    vec4 farPoint = inverseLightSpaceMatrix * vec4(ndc, 1.0, 1.0);
    vec3 rayOrigin = nearPoint.xyz / nearPoint.w;
    vec3 rayEnd = farPoint.xyz / farPoint.w;
    vec3 rayVector = rayEnd - rayOrigin;
    float rayLength = length(rayVector);
    vec3 rayDirection = rayVector / max(rayLength, 0.0001);

    CloudShadowData = vec4(0.0, 1.0, 1.0, 0.0);
    if (abs(rayDirection.y) < 0.0001)
        return;

    float baseT = (cloudBaseHeight - rayOrigin.y) / rayDirection.y;
    float topT = (cloudBaseHeight + cloudThickness - rayOrigin.y) /
                 rayDirection.y;
    float entryT = max(min(baseT, topT), 0.0);
    float exitT = min(max(baseT, topT), rayLength);
    if (exitT <= entryT)
        return;

    float stepLength = (exitT - entryT) /
                       float(CLOUD_SHADOW_STEPS);
    float jitter = shadowJitter(gl_FragCoord.xy);
    float opticalDepth = 0.0;
    float firstInteractionWeight = 0.0;
    float weightedBlockerDepth = 0.0;

    for (int i = 0; i < CLOUD_SHADOW_STEPS; ++i)
    {
        float t = entryT + (float(i) + jitter) * stepLength;
        vec3 worldPosition = rayOrigin + rayDirection * t;
        float density = sharedCloudDensity(worldPosition);
        float deltaOpticalDepth = density * stepLength * 0.001 *
            cloudLightAbsorption * max(cloudShadowStrength, 0.0);
        float transmittanceBefore = exp(-opticalDepth);
        float interactionWeight = transmittanceBefore *
                                  (1.0 - exp(-deltaOpticalDepth));
        vec4 clipPosition = lightSpaceMatrix * vec4(worldPosition, 1.0);
        float blockerDepth = clipPosition.z / clipPosition.w * 0.5 + 0.5;
        weightedBlockerDepth += blockerDepth * interactionWeight;
        firstInteractionWeight += interactionWeight;
        opticalDepth += deltaOpticalDepth;
        if (opticalDepth > 12.0)
            break;
    }

    float transmittance = exp(-opticalDepth);
    float opacity = 1.0 - transmittance;
    float blockerDepth = firstInteractionWeight > 0.000001
        ? weightedBlockerDepth / firstInteractionWeight
        : 1.0;
    CloudShadowData = vec4(
        opticalDepth,
        transmittance,
        clamp(blockerDepth, 0.0, 1.0),
        opacity);
}
