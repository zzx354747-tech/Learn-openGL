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

void main()
{
    vec3 color = hasAlbedoMap ? texture(albedoTexture, TexCoords).rgb : albedoColor;
    if (useTerrainBlend && hasAlbedoMap)
    {
        float blend = smoothstep(0.05, 0.95, texture(terrainBlendTexture, TexCoords1).r);
        color = mix(color, texture(terrainGrassAlbedo, TexCoords * 0.72).rgb, blend);
    }

    color = pow(color, vec3(2.2));
    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
