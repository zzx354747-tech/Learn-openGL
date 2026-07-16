#version 330 core

in vec2 TexCoords;
in vec2 TexCoords1;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform bool hasAlbedoMap;
uniform sampler2D albedoTexture;
uniform vec3 albedoColor;
uniform bool useTerrainBlend;
uniform sampler2D terrainBlendTexture;
uniform sampler2D terrainGrassAlbedo;
uniform sampler2D terrainSnowAlbedo;
uniform bool hasTerrainSnowAlbedo;

void main()
{
    vec3 color = hasAlbedoMap ? texture(albedoTexture, TexCoords).rgb : albedoColor;
    if (useTerrainBlend && hasAlbedoMap)
    {
        vec2 mask = texture(terrainBlendTexture, TexCoords1).rg;
        float grassWeight = clamp(mask.r, 0.0, 1.0);
        float snowWeight = hasTerrainSnowAlbedo
            ? clamp(mask.g, 0.0, 1.0)
            : 0.0;
        grassWeight *= 1.0 - snowWeight;
        float rockWeight = max(1.0 - grassWeight - snowWeight, 0.0);
        color = color * rockWeight +
                texture(terrainGrassAlbedo, TexCoords * 0.72).rgb * grassWeight;
        if (hasTerrainSnowAlbedo)
            color += texture(terrainSnowAlbedo, TexCoords * 0.82).rgb * snowWeight;
    }

    color = pow(color, vec3(2.2));
    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
