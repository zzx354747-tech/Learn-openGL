#version 330 core

in vec3 TexCoords;
in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube skybox;
uniform mat3 iblSunRotation;
uniform float daylightFactor;
uniform sampler2D sunTexture;
uniform sampler2D blueNoiseTexture;
uniform sampler2D cloudAccelerationMap;
uniform sampler2D cloudSunLocalMap;
uniform bool hasBlueNoiseTexture;
uniform bool hasCloudAccelerationMap;
uniform bool hasCloudSunLocalMap;
uniform bool enableCloudOccupancySkipping;
uniform bool enableCloudLightCache;
uniform vec3 cameraPos;
uniform bool isSkybox;
uniform bool enableProceduralSky;
uniform bool enableVolumetricClouds;
uniform bool enableSunTexture;
uniform vec3 skyTopColor;
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
uniform float stormHoleStrength;
uniform int stormHoleSeed;
uniform vec2 stormHoleAnchor;
uniform int stormHoleCount;
uniform float stormHoleMinRadius;
uniform float stormHoleMaxRadius;
uniform float stormHoleSoftness;
uniform float stormHoleShaftStrength;
uniform float cloudEvolutionTime;
uniform vec2 cloudWindOffset;
uniform vec2 cloudWindDirection;
uniform float cloudWindShear;
uniform float cloudExtinction;
uniform float cloudLightAbsorption;
uniform float cloudAmbientStrength;
uniform float cloudPowderStrength;
uniform float cloudMultiScattering;
uniform float cloudSilverLining;
uniform float cloudForwardScattering;
uniform float cloudBackwardScattering;
uniform vec3 cloudSunColor;
uniform vec3 cloudBottomColor;
uniform vec3 cloudTopColor;
uniform int cloudViewSteps;
uniform int cloudLightSteps;
uniform float cloudMaxDistance;
uniform vec2 cloudCacheOrigin;
uniform float cloudCacheWorldSize;
uniform vec2 cloudSunLocalOrigin;
uniform float cloudSunLocalWorldSize;
uniform float cloudOccupancyThreshold;
uniform float cloudEmptySkipMultiplier;
uniform int cloudFrameIndex;
uniform float sunAngularRadius;
uniform float cloudFallbackSunTransmission;

#include "../common/cloud_density.glsl"

const int MAX_VIEW_STEPS = 96;
const int MAX_LIGHT_STEPS = 8;
const int MAX_STORM_HOLES = 7;
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
        // Procedural noise has no hardware mip chain. Gradually removing high
        // octaves with distance prevents sub-pixel cloud detail from turning
        // into moire patterns near the horizon.
        float octaveWeight = 1.0 - smoothstep(
            3.0, 4.0, float(octave) + filterLod);
        sum += valueNoise(p) * amplitude * octaveWeight;
        weightSum += amplitude * octaveWeight;
        p = FBM_ROTATION * p * 2.03 + vec3(7.1, 3.7, 5.9);
        amplitude *= 0.5;
    }
    return sum * (0.975 / max(weightSum, 0.001));
}

float detailFbm(vec3 p)
{
    float sum = 0.0;
    float amplitude = 0.52;
    for (int octave = 0; octave < 3; ++octave)
    {
        sum += valueNoise(p) * amplitude;
        p = FBM_ROTATION * p * 2.07 + vec3(5.3, 9.1, 2.7);
        amplitude *= 0.5;
    }
    return sum;
}

float weatherFbm(vec3 p)
{
    float lowFrequency = valueNoise(p);
    float highFrequency = valueNoise(FBM_ROTATION * p * 2.03 + vec3(7.1, 3.7, 5.9));
    return lowFrequency * 0.67 + highFrequency * 0.33;
}

float interleavedGradientNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

float cloudBlueNoiseJitter()
{
    if (!hasBlueNoiseTexture)
    {
        vec2 temporalOffset = vec2(47.0, 17.0) * float(cloudFrameIndex);
        return interleavedGradientNoise(gl_FragCoord.xy + temporalOffset);
    }

    // The source is a tileable 64x64 CC0 blue-noise texture. A different
    // coprime pixel offset per temporal frame decorrelates ray entry points
    // without the low-frequency clumps produced by white/hash noise.
    ivec2 temporalOffset = ivec2(cloudFrameIndex * 37,
                                 cloudFrameIndex * 17);
    ivec2 noisePixel = (ivec2(gl_FragCoord.xy) + temporalOffset) & ivec2(63);
    return texelFetch(blueNoiseTexture, noisePixel, 0).r;
}

vec2 getWindDirection()
{
    float lengthSquared = dot(cloudWindDirection, cloudWindDirection);
    return lengthSquared > 0.0001
        ? cloudWindDirection * inversesqrt(lengthSquared)
        : vec2(1.0, 0.0);
}

float getCloudHeight01(float worldY)
{
    return (worldY - cloudBaseHeight) / max(cloudThickness, 1.0);
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

    // The complete aperture cluster is anchored once in world space when the
    // pattern is generated. Camera movement only changes how it is viewed.
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
    // Carve through the cloud along the directional light, not vertically.
    lean = sunDirection.xz / max(sunDirection.y, 0.08);
}

float sampleGeneratedStormHole(
    vec2 worldXZ,
    float worldY,
    int index,
    float radiusScale)
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
                 max(radius * radiusScale * axialRadius, 1.0);
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
    // Never erase the density to a perfectly flat zero. Residual wisps make
    // every generated opening participate in the same ray-marched scattering
    // as the primary sunlit aperture.
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
        mask = max(mask, sampleGeneratedStormHole(worldXZ, worldY, i, 1.0));
    }
    return mask;
}

vec2 getCloudSampleXZ(vec3 worldPos, vec2 windOffset, float height01)
{
    vec2 windShear = getWindDirection() * cloudWindShear * cloudThickness * height01;
    vec2 sampleXZ = worldPos.xz + windOffset + windShear;

    // Low-frequency domain warping breaks up the telltale layered fBm pattern
    // and makes large cloud systems curl and fold like a weather front.
    float evolutionTime = cloudEvolutionTime;
    vec2 warpCoord = sampleXZ * (0.00017 * max(cloudScale, 0.01));
    vec2 domainWarp = vec2(
        valueNoise(vec3(warpCoord, 3.7 + evolutionTime * 0.65)),
        valueNoise(vec3(warpCoord + vec2(17.3, -9.1),
                        8.2 - evolutionTime * 0.47))) - 0.5;
    return sampleXZ + domainWarp * (720.0 / max(cloudScale, 0.25));
}

float sampleCloudDensityCoarseAt(vec3 worldPos, vec2 windOffset, out vec2 sampleXZ)
{
    sampleXZ = worldPos.xz + windOffset;
    float height01 = getCloudHeight01(worldPos.y);
    if (height01 <= 0.0 || height01 >= 1.0)
        return 0.0;

    float type = clamp(cloudType, 0.0, 1.0);
    float bottomSoftness = mix(0.055, 0.16, type);
    float topStart = mix(0.54, 0.76, type);
    float verticalProfile = smoothstep(0.0, bottomSoftness, height01) *
                            (1.0 - smoothstep(topStart, 1.0, height01));

    sampleXZ = getCloudSampleXZ(worldPos, windOffset, height01);
    float evolutionTime = cloudEvolutionTime;
    float macroScale = 0.00068 * max(cloudScale, 0.01);
    vec3 samplePos = vec3(sampleXZ * macroScale, height01 * mix(1.55, 2.85, type)).xzy;
    samplePos += vec3(evolutionTime * 0.035,
                      evolutionTime * 0.070,
                     -evolutionTime * 0.025);
    float weather = weatherFbm(vec3(
        sampleXZ * macroScale * 0.24,
        2.4 + evolutionTime * 0.18));
    float sampleDistance = length(worldPos - cameraPos);
    float filterLod = smoothstep(8000.0, 65000.0, sampleDistance) * 2.4;
    float billow = baseFbmFiltered(samplePos, filterLod);
    float threshold = mix(0.72, 0.36, clamp(cloudCoverage, 0.0, 1.0));
    threshold += (0.5 - weather) * 0.24;
    threshold -= cloudAnvilAmount * type * 0.10 * smoothstep(0.42, 0.82, height01);
    float body = smoothstep(threshold - 0.055, threshold + 0.14, billow);
    // Carve the aperture through the complete cloud layer. Because both view
    // and light rays use this same density function, sunlight genuinely sees
    // the opening instead of relying on a screen-space decal.
    float lightHole = sampleStormLightHole(worldPos.xz, worldPos.y) *
                      clamp(stormHoleStrength, 0.0, 1.0);
    body *= 1.0 - lightHole;
    return body * verticalProfile * cloudDensity;
}

float sampleCloudDensityCoarse(vec3 worldPos, vec2 windOffset)
{
    vec2 sampleXZ;
    return sampleCloudDensityCoarseAt(worldPos, windOffset, sampleXZ);
}

float sampleCloudDensity(vec3 worldPos, vec2 windOffset)
{
    vec2 sampleXZ;
    float coarseDensity = sampleCloudDensityCoarseAt(worldPos, windOffset, sampleXZ);
    if (coarseDensity <= 0.004)
        return 0.0;

    float height01 = clamp(getCloudHeight01(worldPos.y), 0.0, 1.0);
    float evolutionTime = cloudEvolutionTime;
    float macroScale = 0.00068 * max(cloudScale, 0.01);
    vec3 samplePos = vec3(sampleXZ * macroScale, height01 * 2.45).xzy;
    vec3 detailEvolution = vec3(evolutionTime * 0.11,
                                -evolutionTime * 0.07,
                                 evolutionTime * 0.09);
    float detail = detailFbm(samplePos * max(cloudDetailScale, 0.1) +
                             vec3(11.0, -4.0, 7.0) + detailEvolution);
    float ridgedDetail = 1.0 - abs(detail * 2.0 - 1.0);
    float erosionNoise = mix(detail, ridgedDetail, 0.35);
    float edgeMask = 1.0 - clamp(coarseDensity, 0.0, 1.0);
    float detailVisibility = 1.0 - smoothstep(
        9000.0, 48000.0, length(worldPos - cameraPos));
    float erosion = (1.0 - erosionNoise) * cloudErosionStrength *
                    mix(0.72, 1.22, height01) * (0.38 + 0.62 * edgeMask) *
                    mix(0.16, 1.0, detailVisibility);
    return max(coarseDensity - erosion * cloudDensity, 0.0);
}

bool sampleCloudAcceleration(
    vec3 worldPos,
    out vec4 acceleration,
    out vec4 penumbraAcceleration)
{
    if (hasCloudSunLocalMap && cloudSunLocalWorldSize > 1.0)
    {
        vec2 localUV =
            (worldPos.xz - cloudSunLocalOrigin) / cloudSunLocalWorldSize;
        vec2 localMargin =
            1.5 / vec2(textureSize(cloudSunLocalMap, 0));
        if (all(greaterThanEqual(localUV, localMargin)) &&
            all(lessThanEqual(localUV, vec2(1.0) - localMargin)))
        {
            acceleration = textureLod(cloudSunLocalMap, localUV, 0.0);
            penumbraAcceleration = textureLod(
                cloudSunLocalMap, localUV, 2.0);
            return true;
        }
    }

    if (!hasCloudAccelerationMap || cloudCacheWorldSize <= 1.0)
        return false;

    vec2 uv = (worldPos.xz - cloudCacheOrigin) / cloudCacheWorldSize;
    vec2 texelMargin = 1.5 / vec2(textureSize(cloudAccelerationMap, 0));
    if (any(lessThan(uv, texelMargin)) ||
        any(greaterThan(uv, vec2(1.0) - texelMargin)))
        return false;

    acceleration = textureLod(cloudAccelerationMap, uv, 0.0);
    penumbraAcceleration = textureLod(cloudAccelerationMap, uv, 1.5);
    return true;
}

float interpolateCachedLight(vec4 acceleration, float height01)
{
    if (height01 <= 0.52)
    {
        float amount = smoothstep(0.22, 0.52, height01);
        return mix(acceleration.g, acceleration.b, amount);
    }
    float amount = smoothstep(0.52, 0.82, height01);
    return mix(acceleration.b, acceleration.a, amount);
}

float lightTransmittance(vec3 position, vec3 lightDir, vec2 windOffset)
{
    vec4 acceleration;
    vec4 penumbraAcceleration;
    if (enableCloudLightCache &&
        sampleCloudAcceleration(
            position, acceleration, penumbraAcceleration))
    {
        float height01 = clamp(getCloudHeight01(position.y), 0.0, 1.0);
        float core = interpolateCachedLight(acceleration, height01);
        float penumbra = interpolateCachedLight(
            penumbraAcceleration, height01);
        return mix(penumbra, core, 0.62);
    }

    float opticalDepth = 0.0;
    int lightSteps = clamp(cloudLightSteps, 2, MAX_LIGHT_STEPS);
    float stepLength = max(55.0, cloudThickness / (float(lightSteps) * 2.2));
    for (int i = 0; i < MAX_LIGHT_STEPS; ++i)
    {
        if (i >= lightSteps)
            break;
        position += lightDir * stepLength;
        // Shadows only need the broad cloud shape. Skipping edge detail here
        // removes most of the secondary-ray noise work without flattening it.
        opticalDepth += sharedCloudDensity(position) * stepLength * 0.001;
        stepLength *= 1.48;
    }
    return exp(-opticalDepth * cloudLightAbsorption);
}

float henyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float inverseRoot = inversesqrt(max(1.0 + g2 - 2.0 * g * cosTheta, 0.001));
    return (1.0 - g2) * inverseRoot * inverseRoot * inverseRoot * 0.07957747;
}

vec4 raymarchClouds(vec3 rayDir)
{
    if (!enableVolumetricClouds || abs(rayDir.y) <= 0.001)
        return vec4(0.0);

    float cloudTop = cloudBaseHeight + max(cloudThickness, 1.0);
    float maxCloudDistance = max(cloudMaxDistance, 1.0);
    float layerT0 = (cloudBaseHeight - cameraPos.y) / rayDir.y;
    float layerT1 = (cloudTop - cameraPos.y) / rayDir.y;
    float nearT = max(min(layerT0, layerT1), 0.0);
    float farT = min(max(layerT0, layerT1), maxCloudDistance);
    if (farT <= nearT)
        return vec4(0.0);

    // A planar cloud slab reaches the maximum ray distance at one identical
    // elevation angle, which otherwise produces a perfectly straight horizon
    // cutoff. Fade the layer well before that hard limit and vary the fade
    // distance slowly by world-facing direction so cloud banks dissolve into
    // the cloud-linked background in broad, irregular groups.
    vec2 horizontalDirection = rayDir.xz /
        max(length(rayDir.xz), 0.001);
    float horizonVariation = valueNoise(vec3(
        horizontalDirection * 5.5 + cameraPos.xz * 0.000018,
        27.0 + cloudEvolutionTime * 0.008));
    float horizonFadeStart = maxCloudDistance *
        mix(0.46, 0.58, horizonVariation);
    float horizonFadeEnd = maxCloudDistance *
        mix(0.78, 0.90, horizonVariation);
    float distantCloudFade = 1.0 - smoothstep(
        horizonFadeStart, horizonFadeEnd, nearT);
    if (distantCloudFade <= 0.001)
        return vec4(0.0);

    // Grazing rays cross a much longer part of the cloud slab, so they need
    // more samples than zenith rays rather than fewer.
    float angleQuality = mix(1.0, 0.68, smoothstep(0.02, 0.38, abs(rayDir.y)));
    int viewSteps = clamp(int(float(cloudViewSteps) * angleQuality), 16, MAX_VIEW_STEPS);
    float stepLength = (farT - nearT) / float(viewSteps);
    float jitter = cloudBlueNoiseJitter();
    float t = nearT + stepLength * jitter;
    float transmittance = 1.0;
    vec3 scattering = vec3(0.0);
    vec2 windOffset = cloudWindOffset;
    float cosTheta = dot(rayDir, sunDirection);
    float phase = 0.62 +
                  cloudSilverLining * henyeyGreenstein(cosTheta, cloudForwardScattering) +
                  0.25 * henyeyGreenstein(cosTheta, cloudBackwardScattering);

    for (int i = 0; i < MAX_VIEW_STEPS; ++i)
    {
        if (i >= viewSteps || t >= farT)
            break;

        vec3 position = cameraPos + rayDir * t;
        vec4 acceleration;
        vec4 penumbraAcceleration;
        bool needsAcceleration = enableCloudOccupancySkipping ||
                                 enableCloudLightCache;
        bool hasAcceleration = needsAcceleration &&
                               sampleCloudAcceleration(
                                   position,
                                   acceleration,
                                   penumbraAcceleration);
        if (enableCloudOccupancySkipping && hasAcceleration &&
            acceleration.r < cloudOccupancyThreshold)
        {
            t += stepLength * max(cloudEmptySkipMultiplier, 1.0);
            continue;
        }
        float density = sharedCloudDensity(position);
        if (density > 0.005)
        {
            float height01 = clamp(getCloudHeight01(position.y), 0.0, 1.0);
            float sunVisibility = enableCloudLightCache && hasAcceleration
                ? mix(
                    interpolateCachedLight(
                        penumbraAcceleration, height01),
                    interpolateCachedLight(acceleration, height01),
                    0.62)
                : lightTransmittance(position, sunDirection, windOffset);
            vec3 ambient = mix(cloudBottomColor, cloudTopColor,
                               smoothstep(0.0, 0.82, height01)) *
                           cloudAmbientStrength *
                           mix(0.07, 1.0, daylightFactor);
            vec3 untintedCloudColor = mix(
                cloudBottomColor, cloudTopColor,
                smoothstep(0.0, 0.82, height01));
            float cloudColorLuminance = dot(
                untintedCloudColor, vec3(0.2126, 0.7152, 0.0722));
            float sunTintEligibility = smoothstep(
                0.07, 0.30, cloudColorLuminance);
            float powder = 1.0 - exp(-density * cloudPowderStrength);
            float scatterEnergy = clamp(cloudMultiScattering, 0.0, 1.0);
            float scatterEnergy2 = scatterEnergy * scatterEnergy;
            float multipleScattering = (sunVisibility +
                                        scatterEnergy * sqrt(max(sunVisibility, 0.0)) +
                                        scatterEnergy2 * sqrt(sqrt(max(sunVisibility, 0.0)))) /
                                       (1.0 + scatterEnergy + scatterEnergy2);
            vec3 directLight = cloudSunColor * multipleScattering * phase *
                               mix(0.72, 1.20, powder) *
                               sunTintEligibility * daylightFactor;
            vec3 lighting = ambient * (0.78 + 0.22 * powder) + directLight;
            float extinction = density * stepLength * 0.001 * cloudExtinction;
            float sampleAlpha = 1.0 - exp(-extinction);
            scattering += transmittance * lighting * sampleAlpha;
            transmittance *= 1.0 - sampleAlpha;
            if (transmittance < 0.018)
                break;
            t += stepLength;
        }
        else
        {
            // Safely advance faster through empty macro cells.
            t += stepLength * 1.55;
        }
    }

    return vec4(
        scattering * distantCloudFade,
        (1.0 - transmittance) * distantCloudFade);
}

void getSunBasis(out vec3 right, out vec3 up)
{
    vec3 upReference = abs(sunDirection.y) > 0.96
        ? vec3(1.0, 0.0, 0.0)
        : vec3(0.0, 1.0, 0.0);
    right = normalize(cross(upReference, sunDirection));
    up = normalize(cross(sunDirection, right));
}

float sunAngularWarp(vec2 offset)
{
    float azimuth = atan(offset.y, offset.x);
    float evolution = cloudEvolutionTime * 0.035;
    return 1.0 +
        sin(azimuth * 3.0 + 0.7 + evolution) * 0.075 +
        sin(azimuth * 7.0 - 1.9 - evolution * 0.7) * 0.038 +
        sin(azimuth * 11.0 + 2.4) * 0.018;
}

float sampleSunAureole(vec3 rayDir)
{
    vec3 right;
    vec3 up;
    getSunBasis(right, up);
    vec2 offset = vec2(dot(rayDir, right), dot(rayDir, up));
    float normalizedRadius = length(offset) /
        max(sunAngularRadius * sunAngularWarp(offset), 0.001);

    // Two smooth Mie-like lobes avoid a hard circular halo boundary. Low-
    // frequency angular modulation represents uneven aerosol/cloud-edge
    // extinction without turning the aureole into a geometric ring.
    float tightLobe = exp(-normalizedRadius * normalizedRadius * 1.85);
    float broadLobe = exp(-normalizedRadius * 1.55);
    float variation = valueNoise(vec3(
        offset / max(sunAngularRadius, 0.001) * 1.7,
        4.1 + cloudEvolutionTime * 0.018));
    float irregularity = mix(0.82, 1.14, variation);
    float front = smoothstep(-0.05, 0.08, dot(rayDir, sunDirection));
    return (tightLobe * 0.58 + broadLobe * 0.42) * irregularity * front;
}

vec4 sampleSun(vec3 rayDir)
{
    if (!enableSunTexture)
        return vec4(0.0);

    vec3 right;
    vec3 up;
    getSunBasis(right, up);
    vec2 offset = vec2(dot(rayDir, right), dot(rayDir, up));
    vec2 uv = offset / max(2.0 * sunAngularRadius, 0.001) + 0.5;
    float normalizedRadius = length(offset) /
        max(sunAngularRadius * sunAngularWarp(offset), 0.001);
    float edge = 1.0 - smoothstep(0.80, 1.16, normalizedRadius);
    if (edge <= 0.0 || any(lessThan(uv, vec2(0.0))) ||
        any(greaterThan(uv, vec2(1.0))))
        return vec4(0.0);
    vec4 sun = texture(sunTexture, uv);
    sun.rgb *= 4.2;
    sun.a *= edge;
    return sun;
}

float sampleSolarDiskVisibleFraction()
{
    if (!enableVolumetricClouds)
        return 1.0;
    if (sunDirection.y <= 0.0001)
        return 0.0;

    // The low cache channel is transmittance from 22% inside the cloud layer
    // toward the sun. At the camera-to-sun intersection it therefore measures
    // the visible solar disk, rather than the visibility of an arbitrary hole.
    float referenceHeight = cloudBaseHeight +
                            max(cloudThickness, 1.0) * 0.22;
    float distanceToLayer =
        (referenceHeight - cameraPos.y) / sunDirection.y;
    if (distanceToLayer <= 0.0)
        return 1.0;

    vec2 diskCenter = cameraPos.xz +
                      sunDirection.xz * distanceToLayer;
    float diskRadiusWorld = distanceToLayer *
                            tan(max(sunAngularRadius, 0.001));

    if (hasCloudSunLocalMap && cloudSunLocalWorldSize > 1.0)
    {
        vec2 uv = (diskCenter - cloudSunLocalOrigin) /
                  cloudSunLocalWorldSize;
        float radiusUv = diskRadiusWorld / cloudSunLocalWorldSize;
        vec2 texelMargin =
            1.5 / vec2(textureSize(cloudSunLocalMap, 0));
        vec2 margin = vec2(radiusUv) + texelMargin;
        if (all(greaterThanEqual(uv, margin)) &&
            all(lessThanEqual(uv, vec2(1.0) - margin)))
        {
            float diameterTexels = max(
                diskRadiusWorld * 2.0 *
                float(textureSize(cloudSunLocalMap, 0).x) /
                cloudSunLocalWorldSize,
                1.0);
            float maximumLod = log2(float(
                min(textureSize(cloudSunLocalMap, 0).x,
                    textureSize(cloudSunLocalMap, 0).y)));
            float coverageLod = clamp(
                log2(diameterTexels * 0.82), 0.0, maximumLod);
            return clamp(textureLod(
                cloudSunLocalMap, uv, coverageLod).g, 0.0, 1.0);
        }
    }

    if (hasCloudAccelerationMap && cloudCacheWorldSize > 1.0)
    {
        vec2 uv = (diskCenter - cloudCacheOrigin) /
                  cloudCacheWorldSize;
        float radiusUv = diskRadiusWorld / cloudCacheWorldSize;
        vec2 texelMargin =
            1.5 / vec2(textureSize(cloudAccelerationMap, 0));
        vec2 margin = vec2(radiusUv) + texelMargin;
        if (all(greaterThanEqual(uv, margin)) &&
            all(lessThanEqual(uv, vec2(1.0) - margin)))
        {
            float diameterTexels = max(
                diskRadiusWorld * 2.0 *
                float(textureSize(cloudAccelerationMap, 0).x) /
                cloudCacheWorldSize,
                1.0);
            float maximumLod = log2(float(
                min(textureSize(cloudAccelerationMap, 0).x,
                    textureSize(cloudAccelerationMap, 0).y)));
            float coverageLod = clamp(
                log2(diameterTexels * 0.82), 0.0, maximumLod);
            return clamp(textureLod(
                cloudAccelerationMap, uv, coverageLod).g, 0.0, 1.0);
        }
    }

    return clamp(cloudFallbackSunTransmission, 0.0, 1.0);
}

vec3 proceduralSky(vec3 rayDir, out float sunContribution, out float godRayMask)
{
    // Keep ordinary weather presets on their selected flat background. During
    // a storm, gradually tint that background from the same bottom/top palette
    // used by the cloud march. The transition follows both storm strength and
    // coverage, so exposed sky and cloud edges no longer read as two layers.
    vec3 nightSky = vec3(0.006, 0.010, 0.024);
    vec3 sky = mix(nightSky, skyTopColor, daylightFactor);
    float stormBackdropWeight = clamp(stormHoleStrength, 0.0, 1.0) *
        smoothstep(0.55, 0.93, clamp(cloudCoverage, 0.0, 1.0));
    float backdropHeight = smoothstep(-0.06, 0.58, rayDir.y);
    vec3 lowStormBackdrop = cloudBottomColor * 0.82;
    vec3 highStormBackdrop = mix(
        cloudBottomColor, cloudTopColor, 0.55) * 0.48;
    vec3 stormBackdrop = mix(
        lowStormBackdrop, highStormBackdrop, backdropHeight);
    stormBackdrop *= mix(0.08, 1.0, daylightFactor);
    sky = mix(sky, stormBackdrop, stormBackdropWeight * 0.88);
    // Lens-facing glare is a solar-disk effect. Its source is evaluated once
    // from the mip-averaged cloud coverage over the complete disk. The power
    // response makes a tiny exposed sliver produce only a tiny glare instead
    // of switching the full aureole on.
    float solarDiskVisibleFraction = sampleSolarDiskVisibleFraction();
    // Storm currently has no lens-facing glare. This only disables the
    // aureole/Bloom response; the solar disk and physical shaft pass remain.
    float weatherGlareVisibility =
        1.0 - smoothstep(0.15, 0.70, stormHoleStrength);
    float solarGlareResponse = pow(
        clamp(solarDiskVisibleFraction, 0.0, 1.0), 1.80) *
        max(stormHoleShaftStrength, 0.0) * weatherGlareVisibility;

    vec4 sun = sampleSun(rayDir);
    sunContribution = sun.a * max(max(sun.r, sun.g), sun.b) *
                      solarGlareResponse;

    float sunAureole = enableSunTexture ? sampleSunAureole(rayDir) : 0.0;
    sky += cloudSunColor * sunAureole * 0.62 * solarGlareResponse;
    sunContribution = max(
        sunContribution,
        sunAureole * 1.65 * solarGlareResponse);

    vec4 clouds = raymarchClouds(rayDir);
    float cloudViewTransmittance = clamp(1.0 - clouds.a, 0.0, 1.0);
    // Treat almost-covered solar texels as covered instead of allowing the
    // HDR disk to punch through a dense cloud with a tiny residual alpha.
    float sunPixelVisibility = smoothstep(
        0.03, 0.92, cloudViewTransmittance);
    sunContribution *= sunPixelVisibility;

    // Apertures and volumetric shafts no longer feed the lens-glare target.
    // Only actual visible solar-disk area controls that target in every
    // weather preset, including Sunny.
    godRayMask = 0.0;
    // Raymarching accumulates premultiplied radiance, so composite it directly.
    sky = sky * (1.0 - clouds.a) + clouds.rgb;
    // The disk is composited after the cloud march and attenuated per pixel.
    // Sun-facing cloud color was already added by the in-cloud scattering
    // model above, so blocking the disk does not make lit clouds colorless.
    sky = mix(sky, sun.rgb, sun.a * sunPixelVisibility);

    return sky;
}

void main()
{
    if (isSkybox)
    {
        vec3 rayDir = normalize(TexCoords);
        float sunContribution = 0.0;
        float godRayMask = 0.0;
        vec3 color = enableProceduralSky
            ? proceduralSky(rayDir, sunContribution, godRayMask)
            : texture(skybox, iblSunRotation * rayDir).rgb;
        FragColor = vec4(color, 1.0);
        BrightColor = vec4(
            color * smoothstep(1.0, 2.2, sunContribution),
            godRayMask);
    }
    else
    {
        vec3 incident = normalize(FragPos - cameraPos);
        vec3 reflected = reflect(incident, normalize(Normal));
        vec3 color = texture(skybox, iblSunRotation * reflected).rgb;
        FragColor = vec4(color, 1.0);
        BrightColor = vec4(0.0);
    }
}
