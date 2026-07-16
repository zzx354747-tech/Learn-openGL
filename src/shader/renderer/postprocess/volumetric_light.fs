#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 Scattering;
layout (location = 1) out vec4 MarchDistance;

uniform sampler2D sceneDepth;
uniform sampler2D directionalShadowMap;
uniform sampler2D cloudOpticalDepthMap;
uniform sampler2D cloudTransmittanceMap;

uniform mat4 inverseViewProjection;
uniform mat4 lightSpaceMatrix;
uniform mat4 cloudShadowMatrix;
uniform vec3 cameraPos;
uniform vec3 towardSun;
uniform vec3 sunRadiance;
uniform float sigmaT;
uniform float sigmaS;
uniform float phaseG;
uniform float maxDistance;
uniform float shadowBias;
uniform int stepCount;
uniform int frameIndex;

uniform float cloudBaseHeight;
uniform float cloudThickness;
uniform float cloudFallbackSunTransmission;
uniform bool hasCloudOpticalDepthMap;

const float PI = 3.14159265359;
const int MAX_STEPS = 64;

vec3 reconstructWorld(vec2 uv, float depth)
{
    vec4 world = inverseViewProjection * vec4(
        uv * 2.0 - 1.0,
        depth * 2.0 - 1.0,
        1.0);
    return world.xyz / max(abs(world.w), 0.000001);
}

float opticalDepthCloudVisibility(
    vec3 worldPosition,
    out float globalTransmittance)
{
    globalTransmittance = cloudFallbackSunTransmission;
    if (!hasCloudOpticalDepthMap)
        return globalTransmittance;

    vec4 lightClip = cloudShadowMatrix * vec4(worldPosition, 1.0);
    vec2 lightNdc = lightClip.xy / max(abs(lightClip.w), 0.000001);
    vec2 uv = clamp(lightNdc * 0.5 + 0.5, vec2(0.0), vec2(1.0));
    float tau = texture(cloudOpticalDepthMap, uv).r;
    float maximumLod = log2(float(textureSize(
        cloudTransmittanceMap, 0).x));
    globalTransmittance = textureLod(
        cloudTransmittanceMap, vec2(0.5), maximumLod).r;
    float edgeDistance = max(abs(lightNdc.x), abs(lightNdc.y));
    float coverageFade = 1.0 - smoothstep(0.85, 1.0, edgeDistance);
    return mix(globalTransmittance, exp(-tau), coverageFade);
}

float henyeyGreenstein(float cosTheta, float g)
{
    float g2 = g * g;
    float denominator = max(1.0 + g2 - 2.0 * g * cosTheta, 0.0001);
    return (1.0 - g2) /
        (4.0 * PI * pow(denominator, 1.5));
}

float bayerJitter()
{
    const float bayer4x4[16] = float[16](
         0.0,  8.0,  2.0, 10.0,
        12.0,  4.0, 14.0,  6.0,
         3.0, 11.0,  1.0,  9.0,
        15.0,  7.0, 13.0,  5.0);
    ivec2 pixel = ivec2(mod(floor(gl_FragCoord.xy), 4.0));
    int index = pixel.x + pixel.y * 4;
    float spatial = (bayer4x4[index] + 0.5) / 16.0;
    return fract(spatial + float(frameIndex) * 0.61803398875);
}

float directionalVisibility(vec3 worldPosition)
{
    // Every ray-march step performs a directional shadow-map lookup.
    vec4 lightClip = lightSpaceMatrix * vec4(worldPosition, 1.0);
    vec3 projected = lightClip.xyz / max(abs(lightClip.w), 0.000001);
    projected = projected * 0.5 + 0.5;
    if (projected.x < 0.0 || projected.x > 1.0 ||
        projected.y < 0.0 || projected.y > 1.0 ||
        projected.z < 0.0 || projected.z > 1.0)
    {
        return 1.0;
    }

    float closestDepth = texture(directionalShadowMap, projected.xy).r;
    return projected.z - shadowBias <= closestDepth ? 1.0 : 0.0;
}

float aerosolDensity(vec3 worldPosition)
{
    float broad = sin(dot(
        worldPosition,
        vec3(0.00043, 0.00071, 0.00029)) + 1.7) * 0.5 + 0.5;
    float detail = sin(dot(
        worldPosition,
        vec3(-0.0011, 0.0017, 0.00083)) - 2.3) * 0.5 + 0.5;
    return mix(0.90, 1.10, broad * 0.68 + detail * 0.32);
}

void main()
{
    vec3 farWorld = reconstructWorld(TexCoords, 1.0);
    vec3 rayDirection = normalize(farWorld - cameraPos);
    float depth = texture(sceneDepth, TexCoords).r;
    float rayLength = maxDistance;
    if (depth < 0.999999)
    {
        vec3 sceneWorld = reconstructWorld(TexCoords, depth);
        rayLength = min(length(sceneWorld - cameraPos), maxDistance);
    }

    int samples = clamp(stepCount, 16, MAX_STEPS);
    float stepLength = rayLength / float(samples);
    float sampleDistance = bayerJitter() * stepLength;
    float transmittance = 1.0;
    vec3 integratedScattering = vec3(0.0);
    float phase = henyeyGreenstein(
        dot(rayDirection, normalize(towardSun)),
        clamp(phaseG, 0.5, 0.76));

    for (int i = 0; i < MAX_STEPS; ++i)
    {
        if (i >= samples || sampleDistance >= rayLength ||
            transmittance < 0.005)
        {
            break;
        }

        vec3 samplePosition = cameraPos + rayDirection * sampleDistance;
        float solidVisibility = directionalVisibility(samplePosition);
        float globalCloudTransmittance = 1.0;
        float cloudVisibility = opticalDepthCloudVisibility(
            samplePosition, globalCloudTransmittance);
        float opening = max(
            cloudVisibility - globalCloudTransmittance, 0.0);
        float shaftVisibility = smoothstep(0.02, 0.30, opening) *
                                cloudVisibility;
        float belowCloud = 1.0 - smoothstep(
            cloudBaseHeight,
            cloudBaseHeight + max(cloudThickness, 1.0) * 0.08,
            samplePosition.y);
        float visibility = solidVisibility * shaftVisibility * belowCloud;
        float mediumDensity = aerosolDensity(samplePosition);
        integratedScattering += transmittance * sigmaS * phase *
                                visibility * mediumDensity * sunRadiance *
                                stepLength;
        transmittance *= exp(-sigmaT * mediumDensity * stepLength);
        sampleDistance += stepLength;
    }

    // Linear HDR radiance; tonemapping happens later in ScreenPass.
    Scattering = vec4(integratedScattering, 0.0);
    MarchDistance = vec4(rayLength, rayLength, rayLength, 1.0);
}
