#version 330 core

in vec3 TexCoords;
in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube skybox;
uniform sampler2D sunTexture;
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
uniform float stormHoleSize;
uniform float stormPoolHoleSize;
uniform float stormHoleSpacing;
uniform float stormHoleSoftness;
uniform float stormHoleShaftStrength;
uniform vec2 stormHeroHolePosition;
uniform vec2 stormShaftLean;
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
uniform int cloudFrameIndex;
uniform float sunAngularRadius;

const int MAX_VIEW_STEPS = 96;
const int MAX_LIGHT_STEPS = 8;
const int STORM_LARGE_HOLE_COUNT = 3;
const vec2 STORM_LARGE_HOLE_OFFSETS[STORM_LARGE_HOLE_COUNT] = vec2[](
    vec2(-3000.0, -15000.0),
    vec2( 3500.0, -18000.0),
    vec2(    0.0, -22000.0));
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

float sampleAuthoredStormHole(vec2 worldXZ, float worldY)
{
    float softness = clamp(stormHoleSoftness, 0.05, 0.80);
    vec2 shaftOffset = stormShaftLean * worldY;

    float poolRadius = max(stormPoolHoleSize, 1.0);
    float poolInnerRadius = poolRadius * (1.0 - softness);
    float poolDistance = length(
        (worldXZ - (stormHeroHolePosition - shaftOffset)) * vec2(0.84, 1.0));
    float authoredHole = 1.0 - smoothstep(
        poolInnerRadius, poolRadius, poolDistance);

    float largeRadius = max(stormHoleSize, 1.0);
    float largeInnerRadius = largeRadius * (1.0 - softness);
    for (int i = 0; i < STORM_LARGE_HOLE_COUNT; ++i)
    {
        vec2 center = stormHeroHolePosition + STORM_LARGE_HOLE_OFFSETS[i] -
                      shaftOffset;
        float largeDistance = length((worldXZ - center) * vec2(0.88, 1.12));
        authoredHole = max(authoredHole, 1.0 - smoothstep(
            largeInnerRadius, largeRadius, largeDistance));
    }
    return authoredHole;
}

float sampleStormLightHole(vec2 worldXZ, float worldY)
{
    if (stormHoleStrength <= 0.001)
        return 0.0;
    // Only the pool opening and the three deliberately placed distant
    // openings are allowed to cut the storm layer.
    return sampleAuthoredStormHole(worldXZ, worldY);
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

float lightTransmittance(vec3 position, vec3 lightDir, vec2 windOffset)
{
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
        opticalDepth += sampleCloudDensityCoarse(position, windOffset) * stepLength * 0.001;
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
    float layerT0 = (cloudBaseHeight - cameraPos.y) / rayDir.y;
    float layerT1 = (cloudTop - cameraPos.y) / rayDir.y;
    float nearT = max(min(layerT0, layerT1), 0.0);
    float farT = min(max(layerT0, layerT1), max(cloudMaxDistance, 1.0));
    if (farT <= nearT)
        return vec4(0.0);

    // Grazing rays cross a much longer part of the cloud slab, so they need
    // more samples than zenith rays rather than fewer.
    float angleQuality = mix(1.0, 0.68, smoothstep(0.02, 0.38, abs(rayDir.y)));
    int viewSteps = clamp(int(float(cloudViewSteps) * angleQuality), 16, MAX_VIEW_STEPS);
    float stepLength = (farT - nearT) / float(viewSteps);
    vec2 temporalOffset = vec2(47.0, 17.0) * float(cloudFrameIndex);
    float jitter = interleavedGradientNoise(gl_FragCoord.xy + temporalOffset);
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
        float density = sampleCloudDensity(position, windOffset);
        if (density > 0.005)
        {
            float sunVisibility = lightTransmittance(position, sunDirection, windOffset);
            float height01 = clamp(getCloudHeight01(position.y), 0.0, 1.0);
            vec3 ambient = mix(cloudBottomColor, cloudTopColor,
                               smoothstep(0.0, 0.82, height01)) * cloudAmbientStrength;
            float powder = 1.0 - exp(-density * cloudPowderStrength);
            float scatterEnergy = clamp(cloudMultiScattering, 0.0, 1.0);
            float scatterEnergy2 = scatterEnergy * scatterEnergy;
            float multipleScattering = (sunVisibility +
                                        scatterEnergy * sqrt(max(sunVisibility, 0.0)) +
                                        scatterEnergy2 * sqrt(sqrt(max(sunVisibility, 0.0)))) /
                                       (1.0 + scatterEnergy + scatterEnergy2);
            vec3 directLight = cloudSunColor * multipleScattering * phase *
                               mix(0.72, 1.20, powder);
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

    float horizonFade = smoothstep(0.008, 0.045, abs(rayDir.y));
    return vec4(scattering * horizonFade, (1.0 - transmittance) * horizonFade);
}

vec4 sampleSun(vec3 rayDir)
{
    if (!enableSunTexture)
        return vec4(0.0);

    vec3 upReference = abs(sunDirection.y) > 0.96 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(upReference, sunDirection));
    vec3 up = normalize(cross(sunDirection, right));
    vec2 offset = vec2(dot(rayDir, right), dot(rayDir, up));
    vec2 uv = offset / max(2.0 * sunAngularRadius, 0.001) + 0.5;
    float front = smoothstep(cos(sunAngularRadius * 1.45), cos(sunAngularRadius * 0.92),
                             dot(rayDir, sunDirection));
    if (front <= 0.0 || any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
        return vec4(0.0);
    vec4 sun = texture(sunTexture, uv);
    sun.rgb *= 4.2;
    sun.a *= front;
    return sun;
}

float sampleStormAperture(vec3 rayDir)
{
    if (stormHoleStrength <= 0.001 || abs(rayDir.y) <= 0.001)
        return 0.0;

    float middleHeight = cloudBaseHeight + cloudThickness * 0.50;
    float distanceToLayer = (middleHeight - cameraPos.y) / rayDir.y;
    if (distanceToLayer <= 0.0 || distanceToLayer > cloudMaxDistance)
        return 0.0;

    vec3 layerPosition = cameraPos + rayDir * distanceToLayer;
    return sampleStormLightHole(layerPosition.xz, layerPosition.y) *
           clamp(stormHoleStrength, 0.0, 1.0);
}

vec3 proceduralSky(vec3 rayDir, out float sunContribution, out float godRayMask)
{
    // The procedural atmosphere intentionally starts from one flat background
    // color. Only the sun, volumetric clouds and linked god rays are layered on
    // top; the HDR cubemap remains available for IBL/reflections.
    vec3 sky = skyTopColor;

    vec4 sun = sampleSun(rayDir);
    sky = mix(sky, sun.rgb, sun.a);
    sunContribution = sun.a * max(max(sun.r, sun.g), sun.b);

    vec4 clouds = raymarchClouds(rayDir);
    float sunHalo = pow(max(dot(rayDir, sunDirection), 0.0), 96.0);
    float normalRaySource = sunHalo * (1.0 - clouds.a);

    // Storm apertures glow independently of the directional sun. Their actual
    // downward columns are integrated in world space by screen.fs, so this
    // mask must no longer pull them toward the projected sun position.
    float aperture = sampleStormAperture(rayDir);
    float apertureVisibility = smoothstep(0.25, 0.92, 1.0 - clouds.a);
    float apertureRaySource = aperture * apertureVisibility *
                              max(stormHoleShaftStrength, 0.0);
    godRayMask = enableSunTexture
        ? normalRaySource * (1.0 - clamp(stormHoleStrength, 0.0, 1.0))
        : 0.0;
    sunContribution = max(sunContribution, apertureRaySource * 1.35);
    // Raymarching accumulates premultiplied radiance, so composite it directly.
    sky = sky * (1.0 - clouds.a) + clouds.rgb;

    // Reveal the sky through the cut without drawing an artificial luminous
    // ring around it. The smooth aperture mask supplies the natural soft edge.
    float apertureCore = smoothstep(0.08, 0.88, aperture);
    vec3 openingSky = mix(skyTopColor, cloudSunColor * 0.95, 0.35);
    sky = mix(sky, openingSky, apertureCore * 0.90);
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
            : texture(skybox, rayDir).rgb;
        FragColor = vec4(color, 1.0);
        BrightColor = vec4(
            color * smoothstep(1.0, 2.2, sunContribution),
            godRayMask);
    }
    else
    {
        vec3 incident = normalize(FragPos - cameraPos);
        vec3 reflected = reflect(incident, normalize(Normal));
        vec3 color = texture(skybox, reflected).rgb;
        FragColor = vec4(color, 1.0);
        BrightColor = vec4(0.0);
    }
}
