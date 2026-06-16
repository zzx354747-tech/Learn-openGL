#version 330 core

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in mat3 TBN;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D parallaxTexture;
uniform sampler2D specularTexture;

uniform bool enableNormalMapping;
uniform bool enableParallaxMapping;
uniform bool hasNormalMap;
uniform bool hasParallaxMap;
uniform bool hasSpecularMap;
uniform float parallaxHeightScale;
uniform float bumpNormalStrength;
uniform int numLayers;

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
    vec2 texCoords = TexCoords;

    if (enableParallaxMapping && hasParallaxMap)
    {
        vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
        texCoords = parallaxMapping(TexCoords, viewDir);
    }

    gPosition = vec4(FragPos, 1.0);

    if (enableNormalMapping && hasNormalMap)
    {
        vec3 normalMap = texture(normalTexture, texCoords).rgb;
        normalMap = normalMap * 2.0 - 1.0; // 将法线从[0,1]范围转换到[-1,1]范围
        gNormal = normalize(TBN * normalMap);
    }
    else if (enableNormalMapping && hasParallaxMap)
    {
        gNormal = normalFromHeightMap(texCoords);
    }
    else
    {
        gNormal = normalize(Normal);
    }

    gAlbedoSpec.rgb = pow(texture(diffuseTexture, texCoords).rgb, vec3(2.2));
    gAlbedoSpec.a = hasSpecularMap ? texture(specularTexture, texCoords).r : 1.0;
}
