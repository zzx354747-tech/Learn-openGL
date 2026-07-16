#version 330 core

layout (location = 0) out float OpticalDepth;

uniform vec2 outputSize;
uniform mat4 inverseCloudShadowMatrix;
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
uniform int cloudShadowMarchSteps;
uniform float stormHoleStrength;
uniform int stormHoleSeed;
uniform vec2 stormHoleAnchor;
uniform int stormHoleCount;
uniform float stormHoleMinRadius;
uniform float stormHoleMaxRadius;
uniform float stormHoleSoftness;

#include "../common/cloud_density.glsl"

const int MAX_SHADOW_STEPS = 8;

void main()
{
    vec2 uv = gl_FragCoord.xy / outputSize;
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 nearPointH = inverseCloudShadowMatrix * vec4(ndc, -1.0, 1.0);
    vec4 farPointH = inverseCloudShadowMatrix * vec4(ndc, 1.0, 1.0);
    vec3 rayOrigin = nearPointH.xyz / nearPointH.w;
    vec3 rayEnd = farPointH.xyz / farPointH.w;
    vec3 rayVector = rayEnd - rayOrigin;
    float rayLength = length(rayVector);
    vec3 rayDirection = rayVector / max(rayLength, 0.000001);

    OpticalDepth = 0.0;
    if (abs(rayDirection.y) <= 0.00001)
        return;

    float baseT = (cloudBaseHeight - rayOrigin.y) / rayDirection.y;
    float topT = (cloudBaseHeight + cloudThickness - rayOrigin.y) /
                 rayDirection.y;
    float entryT = max(min(baseT, topT), 0.0);
    float exitT = min(max(baseT, topT), rayLength);
    if (exitT <= entryT)
        return;

    int steps = clamp(cloudShadowMarchSteps, 4, MAX_SHADOW_STEPS);
    float stepLength = (exitT - entryT) / float(steps);
    float tau = 0.0;
    for (int i = 0; i < MAX_SHADOW_STEPS; ++i)
    {
        if (i >= steps || tau >= 6.0)
            break;
        float t = entryT + (float(i) + 0.5) * stepLength;
        vec3 position = rayOrigin + rayDirection * t;
        tau += sharedCloudCoarseDensity(position) * stepLength * 0.001 *
               cloudLightAbsorption * max(cloudShadowStrength, 0.0);
    }
    OpticalDepth = min(tau, 6.0);
}
