#version 330 core

in vec3 WorldPos;
in vec3 WaterNormal;
in vec2 WaterUV;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube prefilterMap;
uniform sampler2D gPosition;
uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec2 viewportSize;
uniform float waterTime;
uniform float cloudAmbientTransmission;
uniform bool enableStormShaftLighting;
uniform vec2 stormShaftLean;
uniform int stormHoleSeed;
uniform int stormHoleCount;
uniform float stormHoleMinRadius;
uniform float stormHoleMaxRadius;
uniform float stormHoleSoftness;
uniform vec3 stormShaftColor;
uniform float stormShaftSurfaceIntensity;

const int MAX_STORM_HOLES = 7;

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

float sampleGeneratedStormHole(vec2 worldXZ, int index)
{
    vec2 center;
    float radius;
    vec2 ellipseScale;
    float rotation;
    vec2 lean;
    float key;
    getGeneratedStormHole(
        index, center, radius, ellipseScale, rotation, lean, key);
    vec2 local = rotateStormHole(worldXZ - center, rotation) /
                 max(radius * 0.90, 1.0);
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

float sampleStormShaftCluster(vec2 worldXZ)
{
    if (!enableStormShaftLighting || stormShaftSurfaceIntensity <= 0.001)
        return 0.0;

    float mask = 0.0;
    for (int i = 0; i < MAX_STORM_HOLES; ++i)
    {
        if (i >= stormHoleCount)
            break;
        mask = max(mask, sampleGeneratedStormHole(worldXZ, i));
    }
    return mask;
}

vec3 samplePositionSmooth(vec2 uv)
{
    ivec2 size = textureSize(gPosition, 0);
    vec2 pixel = uv * vec2(size) - 0.5;
    ivec2 base = ivec2(floor(pixel));
    base = clamp(base, ivec2(0), size - ivec2(2));
    vec2 f = fract(pixel);
    vec3 a = texelFetch(gPosition, base, 0).rgb;
    vec3 b = texelFetch(gPosition, base + ivec2(1, 0), 0).rgb;
    vec3 c = texelFetch(gPosition, base + ivec2(0, 1), 0).rgb;
    vec3 d = texelFetch(gPosition, base + ivec2(1, 1), 0).rgb;
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main()
{
    vec2 microRipple = vec2(
        sin(dot(WorldPos.xz, vec2(1.71, 0.84)) + waterTime * 2.45) +
        sin(dot(WorldPos.xz, vec2(-0.63, 2.12)) - waterTime * 1.83),
        cos(dot(WorldPos.xz, vec2(0.92, -1.58)) + waterTime * 2.16) +
        sin(dot(WorldPos.xz, vec2(2.34, 0.41)) - waterTime * 2.72));
    vec3 N = normalize(WaterNormal + vec3(microRipple.x, 0.0, microRipple.y) * 0.025);
    vec3 V = normalize(cameraPos - WorldPos);
    vec3 R = reflect(-V, N);
    float fresnel = 0.025 + 0.975 * pow(1.0 - max(dot(N, V), 0.0), 5.0);

    vec2 screenUV = gl_FragCoord.xy / viewportSize;
    vec3 floorPosition = samplePositionSmooth(screenUV);
    float opticalDepth = max(WorldPos.y - floorPosition.y, 0.0);
    float depthFactor = 1.0 - exp(-opticalDepth * 0.32);

    vec3 shallowColor = vec3(0.025, 0.30, 0.27);
    vec3 deepColor = vec3(0.008, 0.055, 0.115);
    vec3 bodyColor = mix(shallowColor, deepColor, depthFactor);
    vec3 reflection = textureLod(prefilterMap, R, 0.75).rgb *
                      cloudAmbientTransmission;

    vec3 L = normalize(-sunDirection);
    vec3 H = normalize(L + V);
    float sunGlint = pow(max(dot(N, H), 0.0), 180.0);
    vec3 color = mix(bodyColor, reflection, clamp(fresnel * 0.82, 0.0, 0.9));
    color += sunColor * sunGlint * 0.16;
    float shaftMask = sampleStormShaftCluster(WorldPos.xz);
    color += stormShaftColor * shaftMask * stormShaftSurfaceIntensity *
             mix(0.18, 0.42, max(N.y, 0.0));

    // Smooth directional foam replaces the former multiplied sine checkerboard.
    float depthAA = max(fwidth(opticalDepth) * 2.0, 0.025);
    float shoreline = smoothstep(0.02, 0.02 + depthAA, opticalDepth) *
                      (1.0 - smoothstep(0.62, 1.28, opticalDepth));
    float foamSignal = 0.50 +
        sin(dot(WorldPos.xz, vec2(0.71, 0.29)) + waterTime * 1.25) * 0.22 +
        sin(dot(WorldPos.xz, vec2(-0.24, 0.93)) - waterTime * 0.82) * 0.18 +
        sin(dot(WorldPos.xz, vec2(1.37, -0.58)) + waterTime * 1.71) * 0.10;
    float foam = shoreline * smoothstep(0.58, 0.82, foamSignal);
    color = mix(color, vec3(0.72, 0.87, 0.84), foam * 0.72);

    float alpha = mix(0.52, 0.84, depthFactor);
    alpha = mix(alpha, 0.94, foam);
    FragColor = vec4(color, alpha);

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 bloomColor = brightness > 1.0 ? color : vec3(0.0);
    BrightColor = vec4(bloomColor, alpha);
}
