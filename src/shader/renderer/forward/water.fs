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
uniform vec2 stormHeroHolePosition;
uniform vec2 stormShaftLean;
uniform float stormHoleSize;
uniform float stormPoolHoleSize;
uniform float stormHoleSoftness;
uniform vec3 stormShaftColor;
uniform float stormShaftSurfaceIntensity;

const int STORM_LARGE_HOLE_COUNT = 3;
const vec2 STORM_LARGE_HOLE_OFFSETS[STORM_LARGE_HOLE_COUNT] = vec2[](
    vec2(-3000.0, -15000.0),
    vec2( 3500.0, -18000.0),
    vec2(    0.0, -22000.0));

float sampleStormShaftCluster(vec2 worldXZ)
{
    if (!enableStormShaftLighting || stormShaftSurfaceIntensity <= 0.001)
        return 0.0;

    float poolRadius = max(stormPoolHoleSize, 1.0);
    float poolInnerRadius = poolRadius *
        (1.0 - clamp(stormHoleSoftness, 0.05, 0.80));
    float poolDistance = length(
        (worldXZ - stormHeroHolePosition) * vec2(0.84, 1.0));
    float mask = 1.0 - smoothstep(
        poolInnerRadius, poolRadius, poolDistance);

    float largeRadius = max(stormHoleSize, 1.0);
    float largeInnerRadius = largeRadius *
        (1.0 - clamp(stormHoleSoftness, 0.05, 0.80));
    for (int i = 0; i < STORM_LARGE_HOLE_COUNT; ++i)
    {
        vec2 center = stormHeroHolePosition + STORM_LARGE_HOLE_OFFSETS[i];
        float distanceToShaft = length((worldXZ - center) * vec2(0.88, 1.12));
        mask = max(mask, 1.0 - smoothstep(
            largeInnerRadius, largeRadius, distanceToShaft));
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
