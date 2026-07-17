#version 330 core

in vec2 TexCoords;
in vec2 TexCoords1;
in vec3 FragPos;
in vec3 Normal;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform bool hasAlbedoMap;
uniform sampler2D albedoTexture;
uniform vec3 albedoColor;
uniform bool useTerrainBlend;
uniform bool hasTerrainData;
uniform sampler2D terrainDataMap;
uniform sampler2D terrainEnvironmentMap;
uniform sampler2D terrainLakeDataMap;
uniform sampler2D terrainNoiseTexture;
uniform bool hasTerrainEnvironment;
uniform bool hasTerrainLakeData;
uniform sampler2D terrainGrassAlbedo;
uniform sampler2D terrainSnowAlbedo;
uniform float u_sunAzimuth;
uniform float u_terrainSunHeightShift;
uniform float u_terrainNoiseHeightShift;
uniform float u_grassEnd;
uniform float u_rockStart;
uniform float u_snowStart;
uniform float u_snowEnd;
uniform float u_terrainTextureScale;
uniform int u_terrainDebugMode;

#include "../common/terrain_biomes.glsl"

vec3 pseudoColor(float v)
{
    return clamp(vec3(1.5 - abs(4.0*v-3.0), 1.5 - abs(4.0*v-2.0),
                      1.5 - abs(4.0*v-1.0)), 0.0, 1.0);
}

void main()
{
    vec3 color = hasAlbedoMap ? texture(albedoTexture, TexCoords).rgb
                              : albedoColor;
    if (useTerrainBlend && hasTerrainData)
    {
        vec4 data = texture(terrainDataMap, TexCoords1);
        float noise = texture(terrainNoiseTexture, FragPos.xz / 34.0).g;
        vec3 weights = biomeWeights(data.r, data.b, noise);
        vec2 environment = hasTerrainEnvironment
            ? texture(terrainEnvironmentMap, TexCoords1).rg
            : vec2(0.5, weights.z);
        if (hasTerrainEnvironment)
        {
            float steepRock = smoothstep(0.32, 0.62, data.g);
            float retainedSnow = environment.y;
            float grassWeight = weights.x * mix(0.58, 1.22, environment.x) *
                                (1.0 - steepRock) * (1.0 - retainedSnow);
            float rockWeight = max(weights.y, steepRock) * (1.0 - retainedSnow);
            weights = vec3(grassWeight, rockWeight, retainedSnow);
            weights /= max(dot(weights, vec3(1.0)), 1e-5);
        }
        float permanentSnowStart = max(u_snowEnd + 0.10, 0.70);
        float permanentSnowEnd = max(u_snowEnd + 0.20, 0.80);
        float pureSnow = smoothstep(permanentSnowStart,
                                    permanentSnowEnd, data.r);
        weights = mix(weights, vec3(0.0, 0.0, 1.0), pureSnow);
        vec3 tri = pow(abs(normalize(Normal)), vec3(4.0));
        tri /= max(dot(tri, vec3(1.0)), 1e-5);
        float s = u_terrainTextureScale;
        vec3 rock = texture(albedoTexture, FragPos.zy*s).rgb*tri.x +
                    texture(albedoTexture, FragPos.xz*s).rgb*tri.y +
                    texture(albedoTexture, FragPos.xy*s).rgb*tri.z;
        vec3 grass = texture(terrainGrassAlbedo, FragPos.xz*s*0.72).rgb;
        vec3 snow = texture(terrainSnowAlbedo, FragPos.xz*s*0.82).rgb;
        color = rock*weights.y + grass*weights.x + snow*weights.z;
        float shoreDistance = hasTerrainLakeData
            ? texture(terrainLakeDataMap, TexCoords1).g : -128.0;
        float shoreBand = smoothstep(-34.0, -2.0, shoreDistance) *
                          (1.0 - smoothstep(-1.0, 2.0, shoreDistance)) *
                          (1.0 - pureSnow);
        vec3 bank = mix(rock, vec3(0.15, 0.11, 0.075), 0.48);
        color = mix(color, bank, shoreBand * 0.86);
        if (u_terrainDebugMode > 0)
        {
            float debugValue = u_terrainDebugMode <= 4
                ? data[u_terrainDebugMode - 1]
                : environment[u_terrainDebugMode - 5];
            color = pseudoColor(debugValue);
        }
    }
    color = pow(color, vec3(2.2));
    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
