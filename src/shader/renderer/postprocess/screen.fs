#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform sampler2D godRaySource;
uniform sampler2D sceneDepth;

uniform float exposure;
uniform bool enableHdr;
uniform bool enableGamma;
uniform bool enableBloom;
uniform float bloomStrength;
uniform bool enableGodRays;
uniform bool enableStormGodRays;
uniform vec2 sunScreenPos;
uniform vec2 invResolution;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPos;
uniform float godRayIntensity;
uniform float screenSpaceGodRayMix;
uniform float godRayDensity;
uniform float godRayDecay;
uniform float godRayWeight;
uniform float godRayExposure;
uniform float godRayRadius;
uniform int godRaySamples;
uniform vec3 godRayColor;
uniform float stormHoleStrength;
uniform float stormHoleSize;
uniform float stormPoolHoleSize;
uniform float stormHoleSpacing;
uniform float stormHoleSoftness;
uniform float stormHoleShaftStrength;
uniform vec2 stormHeroHolePosition;
uniform vec2 stormShaftLean;
uniform float cloudBaseHeight;
uniform float cloudThickness;
uniform float cloudScale;
uniform float cloudEvolutionTime;
uniform vec2 cloudWindOffset;
uniform vec2 cloudWindDirection;
uniform float cloudWindShear;
uniform float cloudMaxDistance;

const int MAX_GOD_RAY_SAMPLES = 64;
const int STORM_SHAFT_SAMPLES = 24;
const int STORM_LARGE_HOLE_COUNT = 3;
const vec2 STORM_LARGE_HOLE_OFFSETS[STORM_LARGE_HOLE_COUNT] = vec2[](
    vec2(-3000.0, -15000.0),
    vec2( 3500.0, -18000.0),
    vec2(    0.0, -22000.0));

float interleavedGradientNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

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

vec2 getWindDirection()
{
    float lengthSquared = dot(cloudWindDirection, cloudWindDirection);
    return lengthSquared > 0.0001
        ? cloudWindDirection * inversesqrt(lengthSquared)
        : vec2(1.0, 0.0);
}

float sampleAuthoredStormHole(vec2 worldXZ, float worldY)
{
    // Keep every volumetric column slightly inside its matching cloud opening.
    // The remaining outer band belongs to the opening itself, so bloom and TAA
    // cannot make the shaft read wider than the hole.
    float shaftRadiusScale = 0.92;
    float softness = max(clamp(stormHoleSoftness, 0.05, 0.80), 0.48);
    vec2 shaftOffset = stormShaftLean * worldY;

    float poolRadius = max(stormPoolHoleSize * shaftRadiusScale, 1.0);
    float poolInnerRadius = poolRadius * (1.0 - softness);
    float poolDistance = length(
        (worldXZ - (stormHeroHolePosition - shaftOffset)) * vec2(0.84, 1.0));
    float authoredHole = 1.0 - smoothstep(
        poolInnerRadius, poolRadius, poolDistance);

    float largeRadius = max(stormHoleSize * shaftRadiusScale, 1.0);
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
    return sampleAuthoredStormHole(worldXZ, worldY);
}

vec3 calculateGodRays()
{
    if (!enableGodRays || godRayIntensity <= 0.0)
        return vec3(0.0);

    int sampleCount = clamp(godRaySamples, 16, MAX_GOD_RAY_SAMPLES);
    vec2 delta = (sunScreenPos - TexCoords) *
                 (godRayDensity / float(sampleCount));
    float jitter = interleavedGradientNoise(
        gl_FragCoord.xy + invResolution * 173.0);
    vec2 sampleUV = TexCoords + delta * jitter;
    float illuminationDecay = 1.0;
    float scattering = 0.0;

    for (int i = 0; i < MAX_GOD_RAY_SAMPLES; ++i)
    {
        if (i >= sampleCount)
            break;

        sampleUV += delta;
        if (any(lessThan(sampleUV, vec2(0.0))) ||
            any(greaterThan(sampleUV, vec2(1.0))))
            break;

        // Skybox leaves depth at the clear value of exactly one. Geometry is
        // therefore a natural occluder, while the source alpha contains the
        // cloud ray-march transmittance written by reflection.fs.
        float skyVisibility = step(0.999999, texture(sceneDepth, sampleUV).r);
        float cloudVisibility = texture(godRaySource, sampleUV).a;
        float distanceToSun = length(sampleUV - sunScreenPos);
        float radialWindow = 1.0 - smoothstep(
            max(godRayRadius, 0.001) * 0.18,
            max(godRayRadius, 0.001),
            distanceToSun);

        scattering += cloudVisibility * skyVisibility * radialWindow *
                      illuminationDecay * godRayWeight;
        illuminationDecay *= godRayDecay;
    }

    return godRayColor * scattering * godRayExposure * godRayIntensity *
           screenSpaceGodRayMix;
}

float integrateStormHeroShaft(
    vec2 center,
    float shaftRadius,
    float visibilityBoost,
    vec3 rayDir,
    float nearT,
    float farT,
    float rayLimit,
    float shaftBottom,
    float shaftLength)
{
    vec2 ellipseScale = vec2(0.88, 1.12);
    vec2 origin = (cameraPos.xz - center + stormShaftLean * cameraPos.y) *
                  ellipseScale;
    vec2 direction = (rayDir.xz + stormShaftLean * rayDir.y) * ellipseScale;
    // Feather only inward. The outer integration boundary remains smaller
    // than the matching cloud aperture.
    float radius = max(shaftRadius * 0.92, 1.0);
    float a = dot(direction, direction);
    float segmentStart = nearT;
    float segmentEnd = farT;

    if (a < 0.000001)
    {
        if (dot(origin, origin) > radius * radius)
            return 0.0;
    }
    else
    {
        float b = 2.0 * dot(origin, direction);
        float c = dot(origin, origin) - radius * radius;
        float discriminant = b * b - 4.0 * a * c;
        if (discriminant <= 0.0)
            return 0.0;
        float root = sqrt(discriminant);
        float cylinderT0 = (-b - root) / (2.0 * a);
        float cylinderT1 = (-b + root) / (2.0 * a);
        segmentStart = max(segmentStart, cylinderT0);
        segmentEnd = min(segmentEnd, cylinderT1);
    }

    segmentStart = max(segmentStart, 0.0);
    segmentEnd = min(segmentEnd, rayLimit);
    float pathLength = max(segmentEnd - segmentStart, 0.0);
    if (pathLength <= 0.0001)
        return 0.0;

    float middleT = (segmentStart + segmentEnd) * 0.5;
    float closestT = middleT;
    if (a >= 0.000001)
        closestT = clamp(-dot(origin, direction) / a,
                         segmentStart, segmentEnd);
    float radialDistance = length(origin + direction * closestT);
    float edgeFeather = 1.0 - smoothstep(
        radius * 0.50, radius, radialDistance);
    vec3 middlePosition = cameraPos + rayDir * middleT;
    float height01 = clamp(
        (middlePosition.y - shaftBottom) / shaftLength, 0.0, 1.0);
    float groundFade = smoothstep(0.0, 0.12, height01);
    float distanceFade = 1.0 - smoothstep(
        rayLimit * 0.58, rayLimit, middleT);
    float dust = mix(0.80, 1.0, valueNoise(
        middlePosition * vec3(0.0014, 0.0021, 0.0014) +
        vec3(0.0, cloudEvolutionTime * 0.035, 0.0)));
    return (pathLength / shaftLength) * groundFade *
           distanceFade * dust * edgeFeather * visibilityBoost * 1.35;
}

vec3 calculateStormGodRays()
{
    if (!enableStormGodRays || stormHoleStrength <= 0.001 ||
        stormHoleShaftStrength <= 0.001)
        return vec3(0.0);

    vec2 clipXY = TexCoords * 2.0 - 1.0;
    vec4 farWorld = inverseViewProjection * vec4(clipXY, 1.0, 1.0);
    farWorld /= max(abs(farWorld.w), 0.00001);
    vec3 rayDir = normalize(farWorld.xyz - cameraPos);

    // The opening sits in the cloud layer, while the illuminated volume is a
    // vertical world-space slab below it. This keeps the column visible when
    // the sun or aperture itself is outside the current screen.
    float shaftTop = cloudBaseHeight + max(cloudThickness, 1.0) * 0.50;
    float shaftBottom = min(
        cloudBaseHeight - max(cloudThickness * 0.85, 1200.0),
        -3000.0);
    float shaftLength = shaftTop - shaftBottom;
    float rayLimit = min(max(cloudMaxDistance, 1.0), 26000.0);

    // Geometry truncates the participating volume, so the shaft can appear in
    // front of terrain without leaking through it.
    float sceneDepthValue = texture(sceneDepth, TexCoords).r;
    if (sceneDepthValue < 0.999999)
    {
        vec4 sceneWorld = inverseViewProjection * vec4(
            clipXY, sceneDepthValue * 2.0 - 1.0, 1.0);
        sceneWorld /= max(abs(sceneWorld.w), 0.00001);
        rayLimit = min(rayLimit, length(sceneWorld.xyz - cameraPos));
    }

    float nearT = 0.0;
    float farT = rayLimit;
    if (abs(rayDir.y) > 0.0005)
    {
        float slabT0 = (shaftBottom - cameraPos.y) / rayDir.y;
        float slabT1 = (shaftTop - cameraPos.y) / rayDir.y;
        nearT = max(min(slabT0, slabT1), 0.0);
        farT = min(max(slabT0, slabT1), rayLimit);
    }
    else if (cameraPos.y < shaftBottom || cameraPos.y > shaftTop)
    {
        return vec3(0.0);
    }

    if (farT <= nearT)
        return vec3(0.0);

    float stepLength = (farT - nearT) / float(STORM_SHAFT_SAMPLES);
    float jitter = interleavedGradientNoise(gl_FragCoord.xy + vec2(91.0, 37.0));
    float t = nearT + stepLength * jitter;
    float scattering = 0.0;

    for (int i = 0; i < STORM_SHAFT_SAMPLES; ++i)
    {
        vec3 position = cameraPos + rayDir * t;
        float aperture = sampleStormLightHole(position.xz, position.y);
        float height01 = clamp((position.y - shaftBottom) / shaftLength, 0.0, 1.0);
        float groundFade = smoothstep(0.0, 0.12, height01);
        float distanceFade = 1.0 - smoothstep(rayLimit * 0.58, rayLimit, t);
        float dust = mix(0.78, 1.0, valueNoise(
            position * vec3(0.0014, 0.0021, 0.0014) +
            vec3(0.0, cloudEvolutionTime * 0.035, 0.0)));
        scattering += aperture * groundFade * distanceFade * dust *
                      (stepLength / shaftLength);
        t += stepLength;
    }

    // Integrate the small pool shaft and the three large distant shafts as
    // analytic cylinders so all remain stable even below the horizon.
    // The camera starts near the pool footprint, so a physically normalized
    // short path is almost invisible from the side. Boost only this small
    // shaft for level/downward views; its radius and footprint are unchanged.
    float upwardView = smoothstep(0.15, 0.75, max(rayDir.y, 0.0));
    float poolVisibilityBoost = mix(14.0, 5.0, upwardView);
    scattering += integrateStormHeroShaft(
        stormHeroHolePosition,
        stormPoolHoleSize,
        poolVisibilityBoost,
        rayDir, nearT, farT, rayLimit, shaftBottom, shaftLength);
    for (int i = 0; i < STORM_LARGE_HOLE_COUNT; ++i)
    {
        scattering += integrateStormHeroShaft(
            stormHeroHolePosition + STORM_LARGE_HOLE_OFFSETS[i],
            stormHoleSize,
            1.0,
            rayDir, nearT, farT, rayLimit, shaftBottom, shaftLength);
    }

    // A mild upward-view phase boost makes the cloud opening read clearly,
    // but the beam direction itself remains fixed along world -Y.
    float viewPhase = mix(0.72, 1.0,
        pow(max(rayDir.y, 0.0), 2.0));
    float strength = clamp(stormHoleStrength, 0.0, 1.0) *
                     max(stormHoleShaftStrength, 0.0) *
                     godRayIntensity * godRayExposure;
    return godRayColor * min(scattering, 2.0) * strength * viewPhase;
}

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    
    // Bloom 叠加
    if (enableBloom)
    {
        vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
        hdrColor += bloomColor * bloomStrength;
    }

    hdrColor += calculateGodRays();
    hdrColor += calculateStormGodRays();

    vec3 result = hdrColor;

    // Tone mapping
    if (enableHdr)
        result = vec3(1.0) - exp(-result * exposure);

    // Gamma correction
    if (enableGamma)
        result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}
