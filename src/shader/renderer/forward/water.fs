#version 330 core

in vec3 WorldPos;
in vec3 WaterNormal;
in vec2 LakeUV;
in float SurfaceWaterLevel;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube prefilterMap;
uniform sampler2D sceneColorOpaque;
uniform sampler2D gPosition;
uniform sampler2D lakeDataMap;
uniform sampler2D shadowMap;
uniform sampler2D cloudOpticalDepthMap;
uniform sampler2D cloudTransmittanceMap;

uniform mat4 view;
uniform mat4 lightSpaceMatrix;
uniform mat4 cloudShadowMatrix;
uniform mat3 iblSunRotation;
uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec2 viewportSize;
uniform vec2 waterWindDirection;
uniform float terrainSize;
uniform float waterTime;
uniform float waterWaveAmplitude;
uniform float waterWavelengthScale;
uniform float detailNormalStrength;
uniform float refractionStrength;
uniform vec3 absorptionCoefficient;
uniform vec3 scatteringColor;
uniform float maxAbsorptionDistance;
uniform float waterRoughness;
uniform float foamShoreWidth;

uniform bool enableDispersion;
uniform vec3 waterIOR_RGB;
uniform float dispersionStrength;
uniform float dispersionBlend;
uniform float dispersionDepthFalloff;
uniform float dispersionMaxPixels;
uniform float spectralGlintStrength;

uniform float cloudAmbientTransmission;
uniform bool hasCloudOpticalDepthMap;
uniform float cloudShadowFallbackTransmission;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

float saturate(float value) { return clamp(value, 0.0, 1.0); }

vec2 perpendicular(vec2 direction)
{
    return vec2(-direction.y, direction.x);
}

vec2 rotateDirection(vec2 direction, float radians)
{
    float c = cos(radians);
    float s = sin(radians);
    return vec2(c * direction.x - s * direction.y,
                s * direction.x + c * direction.y);
}

struct WaveDifferential
{
    float height;
    float dhdx;
    float dhdz;
    float d2hdx2;
    float d2hdz2;
    float d2hdxdz;
};

void accumulateWave(inout WaveDifferential wave, vec2 worldXZ,
                    vec2 direction, float amplitude, float wavelength,
                    float speed, float phase)
{
    float k = TWO_PI / max(wavelength, 0.1);
    float theta = dot(direction, worldXZ) * k + speed * waterTime + phase;
    float sine = sin(theta);
    float cosine = cos(theta);
    wave.height += amplitude * sine;
    wave.dhdx += amplitude * cosine * k * direction.x;
    wave.dhdz += amplitude * cosine * k * direction.y;
    float curvature = -amplitude * sine * k * k;
    wave.d2hdx2 += curvature * direction.x * direction.x;
    wave.d2hdz2 += curvature * direction.y * direction.y;
    wave.d2hdxdz += curvature * direction.x * direction.y;
}

WaveDifferential evaluateWaterWaves(vec2 worldXZ)
{
    vec2 wind = length(waterWindDirection) > 0.0001
        ? normalize(waterWindDirection) : vec2(1.0, 0.0);
    float scale = max(waterWavelengthScale, 0.1);
    WaveDifferential wave = WaveDifferential(0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    accumulateWave(wave, worldXZ, wind,
                   waterWaveAmplitude, 18.0 * scale, 0.82, 0.0);
    accumulateWave(wave, worldXZ, rotateDirection(wind, 0.42),
                   waterWaveAmplitude * 0.48, 10.0 * scale, 1.11, 1.7);
    accumulateWave(wave, worldXZ, rotateDirection(wind, -0.57),
                   waterWaveAmplitude * 0.23, 6.2 * scale, 1.48, 3.1);
    accumulateWave(wave, worldXZ, rotateDirection(wind, 0.86),
                   waterWaveAmplitude * 0.14, 3.8 * scale, 1.92, 5.4);
    return wave;
}

vec2 worldToLakeUV(vec2 worldXZ)
{
    return worldXZ / terrainSize + 0.5;
}

vec3 samplePositionSmooth(vec2 uv)
{
    ivec2 size = textureSize(gPosition, 0);
    vec2 pixel = clamp(uv, vec2(0.0), vec2(1.0)) * vec2(size) - 0.5;
    ivec2 base = clamp(ivec2(floor(pixel)), ivec2(0), size - ivec2(2));
    vec2 f = fract(pixel);
    vec3 a = texelFetch(gPosition, base, 0).rgb;
    vec3 b = texelFetch(gPosition, base + ivec2(1, 0), 0).rgb;
    vec3 c = texelFetch(gPosition, base + ivec2(0, 1), 0).rgb;
    vec3 d = texelFetch(gPosition, base + ivec2(1, 1), 0).rgb;
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

bool validRefractedSample(vec2 uv, vec3 position, float surfaceWaterLevel)
{
    bool validUV = all(greaterThanEqual(uv, vec2(0.001))) &&
                   all(lessThanEqual(uv, vec2(0.999)));
    bool belowWater = position.y < surfaceWaterLevel - 0.01;
    bool behindSurface = distance(cameraPos, position) >
                         distance(cameraPos, WorldPos) + 0.01;
    vec3 lakeData = texture(lakeDataMap, worldToLakeUV(position.xz)).rgb;
    return validUV && belowWater && behindSurface &&
           lakeData.r > 0.01 && lakeData.g >= 0.0 &&
           abs(lakeData.b - surfaceWaterLevel) < 1.0;
}

float cloudShadowVisibility(vec3 worldPosition)
{
    if (!hasCloudOpticalDepthMap)
        return cloudShadowFallbackTransmission;
    vec4 lightClip = cloudShadowMatrix * vec4(worldPosition, 1.0);
    vec2 lightNdc = lightClip.xy / max(abs(lightClip.w), 0.000001);
    vec2 uv = clamp(lightNdc * 0.5 + 0.5, vec2(0.0), vec2(1.0));
    float localTransmittance = exp(-texture(cloudOpticalDepthMap, uv).r);
    float maximumLod = log2(float(textureSize(cloudTransmittanceMap, 0).x));
    float globalTransmittance = textureLod(
        cloudTransmittanceMap, vec2(0.5), maximumLod).r;
    float edgeDistance = max(abs(lightNdc.x), abs(lightNdc.y));
    return mix(globalTransmittance, localTransmittance,
               1.0 - smoothstep(0.85, 1.0, edgeDistance));
}

float directionalShadowVisibility(vec3 worldPosition, vec3 normal, vec3 L)
{
    vec4 clip = lightSpaceMatrix * vec4(worldPosition, 1.0);
    vec3 projected = clip.xyz / max(abs(clip.w), 0.00001);
    projected = projected * 0.5 + 0.5;
    if (projected.z <= 0.0 || projected.z >= 1.0 ||
        any(lessThan(projected.xy, vec2(0.0))) ||
        any(greaterThan(projected.xy, vec2(1.0))))
        return 1.0;
    float bias = max(0.0005, 0.0025 * (1.0 - max(dot(normal, L), 0.0)));
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float occluded = 0.0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            occluded += projected.z - bias >
                texture(shadowMap, projected.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;
    return 1.0 - occluded / 9.0;
}


float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a2 = roughness * roughness;
    a2 *= a2;
    float nDotH = max(dot(N, H), 0.0);
    float denominator = nDotH * nDotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denominator * denominator, 0.00001);
}

void main()
{
    vec3 lakeData = texture(lakeDataMap, LakeUV).rgb;
    float waterDepth = lakeData.r;
    float shoreDistance = lakeData.g;
    float surfaceWaterLevel = SurfaceWaterLevel;
    float shoreAA = max(fwidth(shoreDistance) * 1.5, 0.35);
    float depthAA = max(fwidth(waterDepth) * 1.5, 0.10);
    // Pixel AA removes shimmer; the eight-metre world-space ramp creates the
    // physical shallow-water transition that was missing at the old hard mask.
    float shorelineCoverage = smoothstep(-shoreAA, 8.0, shoreDistance);
    float waterCoverage = shorelineCoverage *
                          smoothstep(0.0, max(depthAA * 2.0, 1.35), waterDepth);
    if (waterCoverage < 0.001)
        discard;

    float shoreFade = smoothstep(1.5, 12.0, shoreDistance);
    float depthFade = smoothstep(0.25, 3.0, waterDepth);
    vec2 microRipple = vec2(
        sin(dot(WorldPos.xz, vec2(1.71, 0.84)) + waterTime * 2.45) +
        sin(dot(WorldPos.xz, vec2(-0.63, 2.12)) - waterTime * 1.83),
        cos(dot(WorldPos.xz, vec2(0.92, -1.58)) + waterTime * 2.16) +
        sin(dot(WorldPos.xz, vec2(2.34, 0.41)) - waterTime * 2.72));
    vec3 N = normalize(WaterNormal + vec3(microRipple.x, 0.0, microRipple.y) *
                       detailNormalStrength * 0.10 * depthFade * shoreFade);
    vec3 V = normalize(cameraPos - WorldPos);
    vec2 screenUV = gl_FragCoord.xy / viewportSize;

    float shallowFoam = 1.0 - smoothstep(0.08, 0.65, waterDepth);
    float shoreFoam = 1.0 - smoothstep(0.0, foamShoreWidth, shoreDistance);
    float foamSignal = 0.50 +
        sin(dot(WorldPos.xz, vec2(0.71, 0.29)) + waterTime * 0.34) * 0.22 +
        sin(dot(WorldPos.xz, vec2(-0.24, 0.93)) - waterTime * 0.27) * 0.18 +
        sin(dot(WorldPos.xz, vec2(1.37, -0.58)) + waterTime * 0.41) * 0.10;
    float foam = shoreFoam * shallowFoam * smoothstep(0.58, 0.82, foamSignal);

    vec3 viewSpacePosition = (view * vec4(WorldPos, 1.0)).xyz;
    vec3 Nvs = normalize(mat3(view) * N);
    vec3 Vvs = normalize(-viewSpacePosition);
    vec3 incidentVS = -Vvs;
    vec3 iorRGB = enableDispersion ? waterIOR_RGB : vec3(1.333);
    vec3 refractR = refract(incidentVS, Nvs, 1.0 / iorRGB.r);
    vec3 refractG = refract(incidentVS, Nvs, 1.0 / iorRGB.g);
    vec3 refractB = refract(incidentVS, Nvs, 1.0 / iorRGB.b);
    vec2 directionR = refractR.xy / max(abs(refractR.z), 0.1);
    vec2 directionG = refractG.xy / max(abs(refractG.z), 0.1);
    vec2 directionB = refractB.xy / max(abs(refractB.z), 0.1);

    float grazing = pow(1.0 - max(dot(N, V), 0.0), 1.5);
    float normalSlope = saturate(length(N.xz) * 2.0);
    float dispersionMask = exp(-waterDepth * dispersionDepthFalloff) *
        mix(0.25, 1.0, grazing) * mix(0.35, 1.0, normalSlope) *
        shoreFade * (1.0 - foam);
    vec2 baseOffset = directionG * refractionStrength * depthFade * shoreFade;
    vec2 uvG = clamp(screenUV + baseOffset, vec2(0.001), vec2(0.999));
    vec2 uvR = uvG;
    vec2 uvB = uvG;
    if (enableDispersion && dispersionStrength > 0.0 &&
        dispersionBlend > 0.0 && dispersionMaxPixels > 0.0)
    {
        // The physical IOR separation is sub-pixel at this render scale. Use
        // the refracted-ray difference for orientation, then express its
        // magnitude explicitly in pixels so the UI controls remain meaningful
        // after TAA and HDR compositing.
        vec2 spectralDirection = directionB - directionR;
        if (dot(spectralDirection, spectralDirection) < 0.000001)
            spectralDirection = Nvs.xy + vec2(0.001, 0.0);
        spectralDirection = normalize(spectralDirection);
        float normalizedIorSpread = clamp(
            abs(waterIOR_RGB.b - waterIOR_RGB.r) / 0.006, 0.0, 4.0);
        float spectralPixels = dispersionMaxPixels * dispersionStrength *
                               normalizedIorSpread * dispersionMask;
        vec2 spectralOffset = spectralDirection * spectralPixels /
                              max(viewportSize, vec2(1.0));
        uvR = uvG + spectralOffset;
        uvB = uvG - spectralOffset;
    }
    uvR = clamp(uvR, vec2(0.001), vec2(0.999));
    uvG = clamp(uvG, vec2(0.001), vec2(0.999));
    uvB = clamp(uvB, vec2(0.001), vec2(0.999));

    vec3 floorG = samplePositionSmooth(uvG);
    if (!validRefractedSample(uvG, floorG, surfaceWaterLevel))
    {
        uvG = screenUV;
        floorG = samplePositionSmooth(uvG);
    }
    vec3 baseSample = texture(sceneColorOpaque, uvG).rgb;
    vec3 dispersedSample = enableDispersion
        ? vec3(texture(sceneColorOpaque, uvR).r,
               baseSample.g,
               texture(sceneColorOpaque, uvB).b)
        : baseSample;
    float luminance = dot(baseSample, vec3(0.2126, 0.7152, 0.0722));
    dispersionMask *= smoothstep(0.03, 0.35, luminance);
    dispersedSample = mix(baseSample, dispersedSample,
                          dispersionBlend * dispersionMask);

    float pathLength = clamp(distance(WorldPos, floorG), 0.0,
                             maxAbsorptionDistance);
    vec3 transmittance = exp(-absorptionCoefficient * pathLength);
    vec3 refractedColor = dispersedSample * transmittance +
                          scatteringColor * (1.0 - transmittance);
    // Caustics belong to the submerged receiver and are applied in deferred
    // lighting. Keeping them out of this surface composition prevents a bright
    // decal from appearing to float on the water plane.

    vec3 reflectionDirection = iblSunRotation * reflect(-V, N);
    vec3 reflectedColor = textureLod(prefilterMap, reflectionDirection,
                                     waterRoughness * 5.0).rgb *
                          cloudAmbientTransmission;
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(N, V), 0.0), 5.0);
    vec3 waterColor = mix(refractedColor, reflectedColor, fresnel);

    vec3 L = normalize(-sunDirection);
    vec3 H = normalize(L + V);
    float nDotL = max(dot(N, L), 0.0);
    float nDotV = max(dot(N, V), 0.0);
    float distribution = distributionGGX(N, H, waterRoughness);
    float k = (waterRoughness + 1.0);
    k = k * k * 0.125;
    float geometryV = nDotV / max(nDotV * (1.0 - k) + k, 0.0001);
    float geometryL = nDotL / max(nDotL * (1.0 - k) + k, 0.0001);
    vec3 fresnelSpecular = vec3(0.02) + vec3(0.98) *
        pow(1.0 - max(dot(H, V), 0.0), 5.0);
    vec3 specular = distribution * geometryV * geometryL * fresnelSpecular /
                    max(4.0 * nDotV * nDotL, 0.0001);
    float sunVisibility = directionalShadowVisibility(WorldPos, N, L) *
                          cloudShadowVisibility(WorldPos);
    waterColor += specular * sunColor * nDotL * sunVisibility;
    float spectralGlint = pow(saturate(max(max(specular.r, specular.g), specular.b)),
                              4.0) * grazing * spectralGlintStrength;
    waterColor += vec3(0.92, 1.0, 1.08) * spectralGlint * sunColor;

    vec3 composed = mix(texture(sceneColorOpaque, screenUV).rgb,
                        waterColor, waterCoverage);
    composed = mix(composed, vec3(0.76, 0.84, 0.82), foam * 0.78);
    FragColor = vec4(composed, 1.0);

    float brightness = dot(composed, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = vec4(brightness > 1.0 ? composed : vec3(0.0), 1.0);
}
