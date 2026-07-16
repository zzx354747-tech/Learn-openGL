#version 330 core

in vec3 WorldPos;
in vec3 WaterNormal;
in vec2 WaterUV;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube prefilterMap;
uniform sampler2D gPosition;
uniform sampler2D cloudOpticalDepthMap;
uniform sampler2D cloudTransmittanceMap;
uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec2 viewportSize;
uniform float waterTime;
uniform float cloudAmbientTransmission;
uniform mat4 cloudShadowMatrix;
uniform bool hasCloudOpticalDepthMap;
uniform float cloudShadowFallbackTransmission;
uniform mat3 iblSunRotation;

float cloudShadowVisibility(vec3 worldPosition)
{
    if (!hasCloudOpticalDepthMap)
        return cloudShadowFallbackTransmission;
    vec4 lightClip = cloudShadowMatrix * vec4(worldPosition, 1.0);
    vec2 lightNdc = lightClip.xy / max(abs(lightClip.w), 0.000001);
    vec2 uv = clamp(lightNdc * 0.5 + 0.5, vec2(0.0), vec2(1.0));
    float localTransmittance = exp(-texture(cloudOpticalDepthMap, uv).r);
    float maximumLod = log2(float(textureSize(
        cloudTransmittanceMap, 0).x));
    float globalTransmittance = textureLod(
        cloudTransmittanceMap, vec2(0.5), maximumLod).r;
    float edgeDistance = max(abs(lightNdc.x), abs(lightNdc.y));
    float coverageFade = 1.0 - smoothstep(0.85, 1.0, edgeDistance);
    return mix(globalTransmittance, localTransmittance, coverageFade);
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
    vec3 dispersionAxis = cross(R, N);
    float dispersionAxisLength = length(dispersionAxis);
    dispersionAxis = dispersionAxisLength > 0.0001
        ? dispersionAxis / dispersionAxisLength
        : vec3(1.0, 0.0, 0.0);
    float surfaceDispersion = mix(
        0.0035, 0.014,
        pow(1.0 - max(dot(N, V), 0.0), 1.5));
    vec3 reflectionDirection = iblSunRotation * R;
    vec3 rotatedDispersionAxis = iblSunRotation * dispersionAxis;
    vec3 reflection = vec3(
        textureLod(prefilterMap,
            normalize(reflectionDirection + rotatedDispersionAxis * surfaceDispersion),
            0.75).r,
        textureLod(prefilterMap, reflectionDirection, 0.75).g,
        textureLod(prefilterMap,
            normalize(reflectionDirection - rotatedDispersionAxis * surfaceDispersion),
            0.75).b) * cloudAmbientTransmission;

    vec3 L = normalize(-sunDirection);
    vec3 H = normalize(L + V);
    float sunGlint = pow(max(dot(N, H), 0.0), 180.0);
    vec3 color = mix(bodyColor, reflection, clamp(fresnel * 0.82, 0.0, 0.9));
    color += sunColor * sunGlint * 0.16 * cloudShadowVisibility(WorldPos);
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
