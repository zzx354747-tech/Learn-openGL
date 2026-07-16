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
uniform int stormHoleSeed;
uniform int stormHoleCount;
uniform float stormHoleMinRadius;
uniform float stormHoleMaxRadius;
uniform float stormHoleSoftness;
uniform float stormHoleShaftStrength;
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
const int MAX_STORM_HOLES = 7;

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
    vec2 leanRandom = vec2(
        stormHash11(key + 12.41),
        stormHash11(key + 15.83));

    float xExtent = index == 0 ? 2800.0 : 9500.0;
    float nearDistance = index == 0 ? 900.0 : 4200.0;
    float farDistance = index == 0 ? 5200.0 : 24000.0;
    center = vec2(
        mix(-xExtent, xExtent, positionX),
       -mix(nearDistance, farDistance, positionZ));

    float minimumRadius = min(stormHoleMinRadius, stormHoleMaxRadius);
    float maximumRadius = max(stormHoleMinRadius, stormHoleMaxRadius);
    radius = mix(minimumRadius, maximumRadius, pow(sizeRandom, 1.35));
    float aspect = mix(0.68, 1.42, aspectRandom);
    ellipseScale = vec2(aspect, 1.0 / aspect);
    rotation = rotationRandom * 6.28318530718;
    lean = stormShaftLean + (leanRandom - 0.5) * 0.028;
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

    vec2 axisCenter = center - lean * worldY;
    vec2 local = rotateStormHole(worldXZ - axisCenter, rotation) /
                 max(radius * radiusScale, 1.0);
    float polarAngle = atan(local.y, local.x);
    float frequencyA = 3.0 + floor(stormHash11(key + 18.29) * 4.0);
    float frequencyB = 7.0 + floor(stormHash11(key + 21.71) * 5.0);
    float phaseA = stormHash11(key + 24.13) * 6.28318530718;
    float phaseB = stormHash11(key + 27.59) * 6.28318530718;
    float amplitudeA = mix(0.055, 0.145, stormHash11(key + 30.31));
    float amplitudeB = mix(0.025, 0.080, stormHash11(key + 33.47));
    float edgeWarp = sin(polarAngle * frequencyA + phaseA) * amplitudeA +
                     sin(polarAngle * frequencyB + phaseB) * amplitudeB;
    float shapedDistance = length(local * ellipseScale) + edgeWarp;
    float softness = max(clamp(stormHoleSoftness, 0.05, 0.80), 0.48);
    return 1.0 - smoothstep(1.0 - softness, 1.0, shapedDistance);
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

float integrateGeneratedStormShaft(
    int holeIndex,
    vec2 center,
    float shaftRadius,
    vec2 ellipseScale,
    float rotation,
    vec2 lean,
    float visibilityBoost,
    vec3 rayDir,
    float nearT,
    float farT,
    float rayLimit,
    float shaftBottom,
    float shaftLength)
{
    vec2 origin = rotateStormHole(
        cameraPos.xz - center + lean * cameraPos.y, rotation) * ellipseScale;
    vec2 direction = rotateStormHole(
        rayDir.xz + lean * rayDir.y, rotation) * ellipseScale;
    // Use only an oversized ellipse as a cheap intersection bound. The actual
    // contribution is clipped below by the exact random aperture function.
    float radius = max(shaftRadius * 0.90 * 1.24, 1.0);
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
    float shapeT0 = mix(segmentStart, closestT, 0.50);
    float shapeT2 = mix(closestT, segmentEnd, 0.50);
    vec3 shapePosition0 = cameraPos + rayDir * shapeT0;
    vec3 shapePosition1 = cameraPos + rayDir * closestT;
    vec3 shapePosition2 = cameraPos + rayDir * shapeT2;
    float apertureShape = max(
        sampleGeneratedStormHole(
            shapePosition0.xz, shapePosition0.y, holeIndex, 0.90),
        max(
            sampleGeneratedStormHole(
                shapePosition1.xz, shapePosition1.y, holeIndex, 0.90),
            sampleGeneratedStormHole(
                shapePosition2.xz, shapePosition2.y, holeIndex, 0.90)));
    if (apertureShape <= 0.001)
        return 0.0;
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
           distanceFade * dust * apertureShape * visibilityBoost * 1.35;
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

    float scattering = 0.0;

    // Intersect every generated shaft, then evaluate that exact hole's warped
    // outline at the closest point. This keeps the volume silhouette identical
    // to the cloud opening instead of overlaying a brighter ellipse.
    float minimumRadius = min(stormHoleMinRadius, stormHoleMaxRadius);
    float maximumRadius = max(stormHoleMinRadius, stormHoleMaxRadius);
    for (int i = 0; i < MAX_STORM_HOLES; ++i)
    {
        if (i >= stormHoleCount)
            break;
        vec2 center;
        float radius;
        vec2 ellipseScale;
        float rotation;
        vec2 lean;
        float key;
        getGeneratedStormHole(
            i, center, radius, ellipseScale, rotation, lean, key);
        float size01 = (radius - minimumRadius) /
            max(maximumRadius - minimumRadius, 1.0);
        float visibilityBoost = mix(
            4.8, 1.0, smoothstep(0.10, 0.82, size01));
        scattering += integrateGeneratedStormShaft(
            i,
            center,
            radius,
            ellipseScale,
            rotation,
            lean,
            visibilityBoost,
            rayDir, nearT, farT, rayLimit, shaftBottom, shaftLength);
    }

    // A mild upward-view phase boost keeps generated openings readable while
    // every shaft remains anchored to its own world-space axis.
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
