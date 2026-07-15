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

uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D parallaxTexture;
uniform sampler2D roughnessTexture;
uniform sampler2D metallicTexture;
uniform sampler2D terrainBlendTexture;
uniform sampler2D terrainGrassAlbedo;
uniform sampler2D terrainGrassNormal;
uniform sampler2D terrainGrassRoughness;
uniform sampler2D terrainGrassMetallic;

// 全局(RenderParams 设)
uniform bool  enableNormalMapping;
uniform bool  enableParallaxMapping;
uniform float bumpNormalStrength;
uniform int   numLayers;
uniform float parallaxHeightScale;

// 材质(Material::bind 设)
uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasParallaxMap;
uniform bool hasRoughnessMap;
uniform bool hasMetallicMap;
uniform bool usePackedMetallicRoughness;
uniform bool useTerrainBlend;
uniform bool hasTerrainGrassNormal;
uniform bool hasTerrainGrassRoughness;
uniform bool hasTerrainGrassMetallic;
uniform bool alphaMask;

uniform vec3  albedoColor;
uniform float roughnessFactor;
uniform float metallicFactor;
uniform float alphaCutoff;

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float layerDepth = 1.0 / float(numLayers);
    vec2 deltaTexCoords = viewDir.xy / viewDir.z * parallaxHeightScale / float(numLayers);

    float currentLayerDepth = 0.0;
    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(parallaxTexture, currentTexCoords).r;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(parallaxTexture, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(parallaxTexture, prevTexCoords).r - (currentLayerDepth - layerDepth);
    float weight = afterDepth / (afterDepth - beforeDepth);

    return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

vec3 normalFromHeightMap(vec2 texCoords)
{
    vec2 texelSize = 1.0 / vec2(textureSize(parallaxTexture, 0));

    float heightLeft  = texture(parallaxTexture, texCoords - vec2(texelSize.x, 0.0)).r;
    float heightRight = texture(parallaxTexture, texCoords + vec2(texelSize.x, 0.0)).r;
    float heightDown  = texture(parallaxTexture, texCoords - vec2(0.0, texelSize.y)).r;
    float heightUp    = texture(parallaxTexture, texCoords + vec2(0.0, texelSize.y)).r;

    vec3 tangentNormal = normalize(vec3(
        (heightLeft - heightRight) * bumpNormalStrength,
        (heightDown - heightUp)   * bumpNormalStrength,
        1.0
    ));

    return normalize(TBN * tangentNormal);
}

void main()
{
    vec2 texCoords = TexCoords;
    vec3 normal;
    float terrainBlend = useTerrainBlend
        ? smoothstep(0.05, 0.95, texture(terrainBlendTexture, TexCoords1).r)
        : 0.0;

    if (enableParallaxMapping && hasParallaxMap)
    {
        vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
        vec2 parallaxOffset = parallaxMapping(texCoords, viewDir) - texCoords;
        texCoords += parallaxOffset;
    }

    vec3 albedo;
    if (hasAlbedoMap)
    {
        vec3 rockAlbedo = texture(albedoTexture, texCoords).rgb;
        vec3 grassAlbedo = useTerrainBlend
            ? texture(terrainGrassAlbedo, texCoords * 0.72).rgb
            : rockAlbedo;
        albedo = pow(mix(rockAlbedo, grassAlbedo, terrainBlend), vec3(2.2));
    }
    else
        albedo = pow(albedoColor, vec3(2.2));

     // 新增：alpha discard，注意这里要用未经 gamma 处理的原始 alpha 通道
    if (alphaMask && hasAlbedoMap)
    {
        float alpha = texture(albedoTexture, texCoords).a;
        if (alpha < alphaCutoff)
            discard;
    }

    gPosition = FragPos;

    if (enableNormalMapping && hasNormalMap)
    {
        vec3 normalMap = texture(normalTexture, texCoords).rgb;
        normalMap = normalMap * 2.0 - 1.0;
        if (useTerrainBlend && hasTerrainGrassNormal)
        {
            vec3 grassNormal = texture(terrainGrassNormal, texCoords * 0.72).rgb * 2.0 - 1.0;
            normalMap = normalize(mix(normalMap, grassNormal, terrainBlend));
        }
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

    float roughness;
    float metallic;
    if (usePackedMetallicRoughness)
    {
        vec3 mr = texture(metallicTexture, texCoords).rgb;
        roughness = mr.g;
        metallic  = mr.b;
    }
    else
    {
        roughness = hasRoughnessMap
            ? texture(roughnessTexture, texCoords).r
            : roughnessFactor;
        metallic  = hasMetallicMap
            ? texture(metallicTexture,  texCoords).r
            : metallicFactor;
    }
    roughness = clamp(roughness, 0.04, 1.0);
    metallic  = clamp(metallic,  0.0,  1.0);
    if (useTerrainBlend)
    {
        float grassRoughness = hasTerrainGrassRoughness
            ? texture(terrainGrassRoughness, texCoords * 0.72).r
            : 0.92;
        float grassMetallic = hasTerrainGrassMetallic
            ? texture(terrainGrassMetallic, texCoords * 0.72).r
            : 0.0;
        roughness = mix(roughness, grassRoughness, terrainBlend);
        metallic = mix(metallic, grassMetallic, terrainBlend);
    }

    // Specular anti-aliasing: normal variation inside one pixel is converted
    // into additional microfacet roughness instead of unstable bright sparks.
    vec3 normalDx = dFdx(normal);
    vec3 normalDy = dFdy(normal);
    float normalVariance = 0.5 * (dot(normalDx, normalDx) + dot(normalDy, normalDy));
    roughness = sqrt(clamp(roughness * roughness + min(normalVariance, 0.32),
                           0.0016, 1.0));

    gNormalRoughness = vec4(normal,  roughness);
    gAlbedoMetallic  = vec4(albedo,  metallic);
}
