#version 330 core

in vec2 TexCoords;
layout (location = 0) out vec4 AccelerationData;

uniform vec2 cloudCacheOrigin;
uniform float cloudCacheWorldSize;
uniform float cloudCacheCellSize;
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
uniform int cloudCacheLightSteps;
uniform float stormHoleStrength;
uniform int stormHoleSeed;
uniform vec2 stormHoleAnchor;
uniform int stormHoleCount;
uniform float stormHoleMinRadius;
uniform float stormHoleMaxRadius;
uniform float stormHoleSoftness;

#include "../common/cloud_density.glsl"

const int MAX_STORM_HOLES = 7;
const int MAX_CACHE_LIGHT_STEPS = 6;
const mat3 FBM_ROTATION = mat3(0.00, 0.80, 0.60,
                               -0.80, 0.36, -0.48,
                               -0.60, -0.48, 0.64);

float hash31(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float valueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash31(i + vec3(0, 0, 0)), hash31(i + vec3(1, 0, 0)), f.x),
            mix(hash31(i + vec3(0, 1, 0)), hash31(i + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(hash31(i + vec3(0, 0, 1)), hash31(i + vec3(1, 0, 1)), f.x),
            mix(hash31(i + vec3(0, 1, 1)), hash31(i + vec3(1, 1, 1)), f.x), f.y),
        f.z);
}

float baseFbmFiltered(vec3 p, float filterLod)
{
    float sum = 0.0;
    float weightSum = 0.0;
    float amplitude = 0.52;
    for (int octave = 0; octave < 4; ++octave)
    {
        float octaveWeight = 1.0 - smoothstep(
            3.0, 4.0, float(octave) + filterLod);
        sum += valueNoise(p) * amplitude * octaveWeight;
        weightSum += amplitude * octaveWeight;
        p = FBM_ROTATION * p * 2.03 + vec3(7.1, 3.7, 5.9);
        amplitude *= 0.5;
    }
    return sum * (0.975 / max(weightSum, 0.001));
}

float weatherFbm(vec3 p)
{
    float lowFrequency = valueNoise(p);
    float highFrequency = valueNoise(
        FBM_ROTATION * p * 2.03 + vec3(7.1, 3.7, 5.9));
    return lowFrequency * 0.67 + highFrequency * 0.33;
}

vec2 getWindDirection()
{
    float lengthSquared = dot(cloudWindDirection, cloudWindDirection);
    return lengthSquared > 0.0001
        ? cloudWindDirection * inversesqrt(lengthSquared)
        : vec2(1.0, 0.0);
}

float stormHash11(float value)
{
    return fract(sin(value * 12.9898 + 31.416) * 43758.5453123);
}

vec2 rotateStormHole(vec2 value, float angle)
{
    float sine = sin(angle);
    float cosine = cos(angle);
    return vec2(
        cosine * value.x + sine * value.y,
       -sine * value.x + cosine * value.y);
}

void getGeneratedStormHole(
    int index,
    out vec2 center,
    out float radius,
    out vec2 ellipseScale,
    out float rotation,
    out vec2 lean,
    out float key)
{
    key = float(stormHoleSeed) * 0.071 + float(index) * 17.137;
    float positionX = stormHash11(key + 0.37);
    float positionZ = stormHash11(key + 2.11);
    float sizeRandom = stormHash11(key + 4.73);
    float aspectRandom = stormHash11(key + 7.19);
    float rotationRandom = stormHash11(key + 9.97);
    float minimumRadius = min(stormHoleMinRadius, stormHoleMaxRadius);
    float maximumRadius = max(stormHoleMinRadius, stormHoleMaxRadius);
    radius = index == 0
        ? mix(max(minimumRadius, maximumRadius * 0.62),
              maximumRadius, sizeRandom)
        : (index < 5
            ? mix(max(minimumRadius, maximumRadius * 0.52),
                  maximumRadius * 0.88, sizeRandom)
            : mix(minimumRadius, maximumRadius, pow(sizeRandom, 1.35)));

    vec2 sunLayerCenter = stormHoleAnchor;
    if (index == 0)
    {
        center = sunLayerCenter;
    }
    else if (index < 5)
    {
        float clusterAngle = 6.28318530718 *
            (float(index) / 5.0 + positionX * 0.12);
        float clusterDistance = mix(
            maximumRadius * 1.35,
            maximumRadius * 4.50,
            positionZ);
        center = sunLayerCenter +
            vec2(cos(clusterAngle), sin(clusterAngle)) * clusterDistance;
    }
    else
    {
        float clusterAngle = 6.28318530718 *
            (float(index) / float(max(stormHoleCount, 1)) +
             positionX * 0.12);
        float clusterDistance = mix(
            maximumRadius * 2.2,
            maximumRadius * 5.6,
            positionZ);
        center = sunLayerCenter +
            vec2(cos(clusterAngle), sin(clusterAngle)) * clusterDistance;
    }

    float aspect = mix(0.68, 1.42, aspectRandom);
    ellipseScale = vec2(aspect, 1.0 / aspect);
    rotation = rotationRandom * 6.28318530718;
    lean = sunDirection.xz / max(sunDirection.y, 0.08);
}

float sampleGeneratedStormHole(vec2 worldXZ, float worldY, int index)
{
    vec2 center;
    float radius;
    vec2 ellipseScale;
    float rotation;
    vec2 lean;
    float key;
    getGeneratedStormHole(
        index, center, radius, ellipseScale, rotation, lean, key);
    float middleHeight = cloudBaseHeight + cloudThickness * 0.50;
    vec2 axisCenter = center + lean * (worldY - middleHeight);
    float height01 = clamp(
        (worldY - cloudBaseHeight) / max(cloudThickness, 1.0),
        0.0, 1.0);
    float axialNoise = valueNoise(vec3(
        vec2(key * 0.013, key * 0.021),
        height01 * 2.7 + cloudEvolutionTime * 0.012));
    float axialRadius = mix(0.82, 1.12, axialNoise);
    vec2 local = rotateStormHole(worldXZ - axisCenter, rotation) /
                 max(radius * axialRadius, 1.0);
    float polarAngle = atan(local.y, local.x);
    float frequencyA = 3.0 + floor(stormHash11(key + 18.29) * 4.0);
    float frequencyB = 7.0 + floor(stormHash11(key + 21.71) * 5.0);
    float phaseA = stormHash11(key + 24.13) * 6.28318530718;
    float phaseB = stormHash11(key + 27.59) * 6.28318530718;
    float amplitudeA = mix(0.055, 0.145, stormHash11(key + 30.31));
    float amplitudeB = mix(0.025, 0.080, stormHash11(key + 33.47));
    float edgeWarp = sin(polarAngle * frequencyA + phaseA) * amplitudeA +
                     sin(polarAngle * frequencyB + phaseB) * amplitudeB;
    float turbulentEdge = valueNoise(vec3(
        worldXZ * 0.00072,
        key * 0.019 + height01 * 2.1 + cloudEvolutionTime * 0.009));
    float shapedDistance = length(local * ellipseScale) + edgeWarp +
                           (turbulentEdge - 0.5) * 0.24;
    float softness = clamp(stormHoleSoftness, 0.05, 0.80);
    float aperture = 1.0 - smoothstep(
        1.0 - softness, 1.0, shapedDistance);
    float interiorStructure = valueNoise(vec3(
        worldXZ * 0.00135 + vec2(key * 0.007),
        height01 * 4.2 - cloudEvolutionTime * 0.016));
    float carveStrength = mix(0.78, 0.97, interiorStructure);
    return aperture * carveStrength;
}

float sampleStormLightHole(vec2 worldXZ, float worldY)
{
    if (stormHoleStrength <= 0.001)
        return 0.0;
    float mask = 0.0;
    for (int i = 0; i < MAX_STORM_HOLES; ++i)
    {
        if (i >= stormHoleCount)
            break;
        mask = max(mask, sampleGeneratedStormHole(worldXZ, worldY, i));
    }
    return mask;
}

vec2 getCloudSampleXZ(vec3 worldPos, float height01)
{
    vec2 windShear = getWindDirection() * cloudWindShear *
                     cloudThickness * height01;
    vec2 sampleXZ = worldPos.xz + cloudWindOffset + windShear;
    vec2 warpCoord = sampleXZ * (0.00017 * max(cloudScale, 0.01));
    vec2 domainWarp = vec2(
        valueNoise(vec3(warpCoord, 3.7 + cloudEvolutionTime * 0.65)),
        valueNoise(vec3(warpCoord + vec2(17.3, -9.1),
                        8.2 - cloudEvolutionTime * 0.47))) - 0.5;
    return sampleXZ + domainWarp * (720.0 / max(cloudScale, 0.25));
}

float sampleCloudDensityCoarse(vec3 worldPos)
{
    float height01 = (worldPos.y - cloudBaseHeight) /
                     max(cloudThickness, 1.0);
    if (height01 <= 0.0 || height01 >= 1.0)
        return 0.0;

    float type = clamp(cloudType, 0.0, 1.0);
    float bottomSoftness = mix(0.055, 0.16, type);
    float topStart = mix(0.54, 0.76, type);
    float verticalProfile = smoothstep(0.0, bottomSoftness, height01) *
                            (1.0 - smoothstep(topStart, 1.0, height01));
    vec2 sampleXZ = getCloudSampleXZ(worldPos, height01);
    float macroScale = 0.00068 * max(cloudScale, 0.01);
    vec3 samplePos = vec3(
        sampleXZ * macroScale,
        height01 * mix(1.55, 2.85, type)).xzy;
    samplePos += vec3(cloudEvolutionTime * 0.035,
                      cloudEvolutionTime * 0.070,
                     -cloudEvolutionTime * 0.025);
    float weather = weatherFbm(vec3(
        sampleXZ * macroScale * 0.24,
        2.4 + cloudEvolutionTime * 0.18));
    float sampleDistance = length(worldPos - cameraPos);
    float filterLod = smoothstep(8000.0, 65000.0, sampleDistance) * 2.4;
    float billow = baseFbmFiltered(samplePos, filterLod);
    float threshold = mix(0.72, 0.36, clamp(cloudCoverage, 0.0, 1.0));
    threshold += (0.5 - weather) * 0.24;
    threshold -= cloudAnvilAmount * type * 0.10 *
                 smoothstep(0.42, 0.82, height01);
    float body = smoothstep(threshold - 0.055, threshold + 0.14, billow);
    float lightHole = sampleStormLightHole(worldPos.xz, worldPos.y) *
                      clamp(stormHoleStrength, 0.0, 1.0);
    body *= 1.0 - lightHole;
    return body * verticalProfile * cloudDensity;
}

float cachedLightTransmittance(vec2 worldXZ, float height01)
{
    vec3 position = vec3(
        worldXZ.x,
        cloudBaseHeight + cloudThickness * height01,
        worldXZ.y);
    float opticalDepth = 0.0;
    int lightSteps = clamp(
        cloudCacheLightSteps, 2, MAX_CACHE_LIGHT_STEPS);
    float stepLength = max(
        80.0, cloudThickness / (float(lightSteps) * 1.8));
    for (int i = 0; i < MAX_CACHE_LIGHT_STEPS; ++i)
    {
        if (i >= lightSteps)
            break;
        position += sunDirection * stepLength;
        opticalDepth += sharedCloudDensity(position) *
                        stepLength * 0.001;
        stepLength *= 1.42;
    }
    return exp(-opticalDepth * cloudLightAbsorption);
}

void main()
{
    vec2 worldXZ = cloudCacheOrigin +
                   TexCoords * cloudCacheWorldSize;
    float maximumDensity = 0.0;

    // Two XZ positions and eight heights form a conservative column maximum.
    // The second position is offset inside the cache cell to retain cloud banks
    // narrower than a single cache texel.
    for (int heightIndex = 0; heightIndex < 8; ++heightIndex)
    {
        float height01 = (float(heightIndex) + 0.5) / 8.0;
        float worldY = cloudBaseHeight + cloudThickness * height01;
        maximumDensity = max(maximumDensity,
            sharedCloudDensity(vec3(worldXZ.x, worldY, worldXZ.y)));
        vec2 offsetXZ = worldXZ +
            vec2(0.31, -0.27) * cloudCacheCellSize;
        maximumDensity = max(maximumDensity,
            sharedCloudDensity(vec3(offsetXZ.x, worldY, offsetXZ.y)));
    }

    float lowLight = cachedLightTransmittance(worldXZ, 0.22);
    float middleLight = cachedLightTransmittance(worldXZ, 0.52);
    float highLight = cachedLightTransmittance(worldXZ, 0.82);
    AccelerationData = vec4(
        maximumDensity, lowLight, middleLight, highLight);
}
