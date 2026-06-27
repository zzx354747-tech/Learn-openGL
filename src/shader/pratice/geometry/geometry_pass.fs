#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormalRoughness;
layout (location = 2) out vec4 gAlbedoMetallic;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec2 TexCoords1;
in mat3 TBN;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

uniform sampler2D normalTexture;
uniform sampler2D parallaxTexture;
uniform sampler2D albedoTexture;
uniform sampler2D metallicTexture;
uniform sampler2D roughnessTexture;

uniform bool enableNormalMapping;
uniform bool enableParallaxMapping;
uniform bool hasNormalMap;
uniform bool hasParallaxMap;
uniform bool usePackedMetallicRoughness;
uniform bool hasRoughnessMap;
uniform bool hasMetallicMap;
uniform vec4 baseColorFactor;
uniform float roughnessFactor;
uniform float metallicFactor;
uniform bool alphaMask;
uniform float alphaCutoff;
uniform int albedoTexCoordIndex;
uniform int normalTexCoordIndex;
uniform int parallaxTexCoordIndex;
uniform int roughnessTexCoordIndex;
uniform int metallicTexCoordIndex;
uniform float parallaxHeightScale;
uniform float bumpNormalStrength;
uniform int numLayers;

vec2 getTexCoords(int index)
{
    return index == 1 ? TexCoords1 : TexCoords;
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // 计算步长
    float layerDepth = 1.0 / float(numLayers);
    // 计算单层uv偏移量
    vec2 deltaTexCoords = viewDir.xy / viewDir.z * parallaxHeightScale / float(numLayers);

    // 初始化当前层的深度
    float currentLayerDepth = 0.0;
    // 初始化偏移前的纹理坐标
    vec2 currentTexCoords = texCoords;
    // 获取当前层的深度值
    float currentDepthMapValue = texture(parallaxTexture, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        // 更新当前层uv
        currentTexCoords -= deltaTexCoords;
        // 更新当前层深度值
        currentDepthMapValue = texture(parallaxTexture, currentTexCoords).r;
        // 更新当前层深度
        currentLayerDepth += layerDepth;
    }

    // 线性插值，获得更精确的纹理坐标
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(parallaxTexture, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);

    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

vec3 normalFromHeightMap(vec2 texCoords)
{
    vec2 texelSize = 1.0 / vec2(textureSize(parallaxTexture, 0));

    float heightLeft = texture(parallaxTexture, texCoords - vec2(texelSize.x, 0.0)).r;
    float heightRight = texture(parallaxTexture, texCoords + vec2(texelSize.x, 0.0)).r;
    float heightDown = texture(parallaxTexture, texCoords - vec2(0.0, texelSize.y)).r;
    float heightUp = texture(parallaxTexture, texCoords + vec2(0.0, texelSize.y)).r;

    vec3 tangentNormal = normalize(vec3(
        (heightLeft - heightRight) * bumpNormalStrength,
        (heightDown - heightUp) * bumpNormalStrength,
        1.0
    ));

    return normalize(TBN * tangentNormal);
}

void main()
{
    vec2 texCoords = getTexCoords(albedoTexCoordIndex);
    vec2 normalTexCoords = getTexCoords(normalTexCoordIndex);
    vec2 parallaxTexCoords = getTexCoords(parallaxTexCoordIndex);
    vec2 roughnessTexCoords = getTexCoords(roughnessTexCoordIndex);
    vec2 metallicTexCoords = getTexCoords(metallicTexCoordIndex);
    vec3 normal;

    if (enableParallaxMapping && hasParallaxMap)
    {
        vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
        vec2 parallaxOffset = parallaxMapping(parallaxTexCoords, viewDir) - parallaxTexCoords;
        texCoords += parallaxOffset;
        normalTexCoords += parallaxOffset;
        roughnessTexCoords += parallaxOffset;
        metallicTexCoords += parallaxOffset;
    }

    gPosition = vec3(FragPos);

    if (enableNormalMapping && hasNormalMap)
    {
        vec3 normalMap = texture(normalTexture, normalTexCoords).rgb;
        normalMap = normalMap * 2.0 - 1.0; // 将法线从[0,1]范围转换到[-1,1]范围
        normalMap = normalize(mix(vec3(0.0, 0.0, 1.0), normalMap, bumpNormalStrength));
        normal = normalize(TBN * normalMap);
    }
    else if (enableNormalMapping && hasParallaxMap)
    {
        normal = normalFromHeightMap(texCoords);
    }
    else
    {
        normal = normalize(Normal);
    }

    vec3 metallicRoughness = texture(metallicTexture, metallicTexCoords).rgb;
    float roughness = usePackedMetallicRoughness
        ? metallicRoughness.g
        : (hasRoughnessMap ? texture(roughnessTexture, roughnessTexCoords).r : roughnessFactor);
    float metallic = usePackedMetallicRoughness
        ? metallicRoughness.b
        : (hasMetallicMap ? metallicRoughness.r : metallicFactor);
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    vec4 albedoSample = texture(albedoTexture, texCoords) * baseColorFactor;
    if (alphaMask && albedoSample.a < alphaCutoff)
    {
        discard;
    }
    vec3 albedo = pow(albedoSample.rgb, vec3(2.2));

    gNormalRoughness = vec4(normal, roughness);
    gAlbedoMetallic = vec4(albedo, metallic);

}
