#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform sampler2D lowResolutionScattering;
uniform sampler2D lowResolutionDistance;
uniform sampler2D sceneDepth;
uniform mat4 inverseViewProjection;
uniform vec3 cameraPos;
uniform float maxDistance;
uniform float depthSigma;

vec3 reconstructWorld(vec2 uv, float depth)
{
    vec4 world = inverseViewProjection * vec4(
        uv * 2.0 - 1.0,
        depth * 2.0 - 1.0,
        1.0);
    return world.xyz / max(abs(world.w), 0.000001);
}

float fullResolutionDistance(float depth)
{
    if (depth >= 0.999999)
        return maxDistance;
    return min(
        length(reconstructWorld(TexCoords, depth) - cameraPos),
        maxDistance);
}

void main()
{
    ivec2 lowSize = textureSize(lowResolutionScattering, 0);
    vec2 lowPosition = TexCoords * vec2(lowSize) - 0.5;
    ivec2 center = ivec2(floor(lowPosition));
    float referenceDistance = fullResolutionDistance(
        texture(sceneDepth, TexCoords).r);
    float rejectionWidth = max(0.5, referenceDistance * depthSigma);

    vec3 scattering = vec3(0.0);
    float weightSum = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            ivec2 sampleCoord = clamp(
                center + ivec2(x, y),
                ivec2(0),
                lowSize - ivec2(1));
            vec2 sampleCenter = vec2(sampleCoord) + 0.5;
            vec2 offset = sampleCenter - (lowPosition + 0.5);
            float spatialWeight = exp(-dot(offset, offset) * 0.72);
            float lowDistance = texelFetch(
                lowResolutionDistance, sampleCoord, 0).r;
            float distanceDelta = abs(lowDistance - referenceDistance);
            if (distanceDelta > rejectionWidth * 4.0)
                continue;
            float depthWeight = exp(-distanceDelta / rejectionWidth);
            float weight = spatialWeight * depthWeight;
            scattering += texelFetch(
                lowResolutionScattering, sampleCoord, 0).rgb * weight;
            weightSum += weight;
        }
    }

    if (weightSum > 0.00001)
        scattering /= weightSum;
    else
        scattering = vec3(0.0);

    FragColor = vec4(scattering, 0.0);
    // Volumetric shafts remain linear HDR scene radiance, but they are not a
    // lens-facing glare source. Solar glare is extracted by the sky pass from
    // the cloud-covered area of the solar disk.
    BrightColor = vec4(0.0);
}
