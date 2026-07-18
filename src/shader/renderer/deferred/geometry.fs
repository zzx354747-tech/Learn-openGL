#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gNormalRoughness;
layout (location = 2) out vec4 gAlbedoMetallic;
layout (location = 3) out vec2 gVelocity;
layout (location = 4) out vec2 gCoverageReactive;

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
uniform sampler2D terrainDataMap;
uniform sampler2D terrainEnvironmentMap;
uniform sampler2D terrainLakeDataMap;
uniform sampler2D terrainNoiseTexture;
uniform sampler2D terrainRockHeight;
uniform sampler2D terrainGrassAlbedo;
uniform sampler2D terrainGrassNormal;
uniform sampler2D terrainGrassRoughness;
uniform sampler2D terrainGrassMetallic;
uniform sampler2D terrainGrassHeight;
uniform sampler2D terrainSnowAlbedo;
uniform sampler2D terrainSnowNormal;
uniform sampler2D terrainSnowRoughness;
uniform sampler2D terrainSnowHeight;

uniform bool enableNormalMapping;
uniform bool enableParallaxMapping;
uniform float bumpNormalStrength;
uniform int numLayers;
uniform float parallaxHeightScale;
uniform bool hasAlbedoMap;
uniform bool hasNormalMap;
uniform bool hasParallaxMap;
uniform bool hasRoughnessMap;
uniform bool hasMetallicMap;
uniform bool usePackedMetallicRoughness;
uniform bool useTerrainBlend;
uniform bool hasTerrainData;
uniform bool hasTerrainEnvironment;
uniform bool hasTerrainLakeData;
uniform bool hasTerrainGrassNormal;
uniform bool hasTerrainGrassRoughness;
uniform bool hasTerrainGrassMetallic;
uniform bool hasTerrainSnowAlbedo;
uniform bool hasTerrainSnowNormal;
uniform bool hasTerrainSnowRoughness;
uniform bool alphaMask;
uniform vec3 albedoColor;
uniform float roughnessFactor;
uniform float metallicFactor;
uniform float alphaCutoff;

uniform float u_sunAzimuth;
uniform float u_terrainSunHeightShift;
uniform float u_terrainNoiseHeightShift;
uniform float u_grassEnd;
uniform float u_rockStart;
uniform float u_snowStart;
uniform float u_snowEnd;
uniform float u_terrainBlendSharpness;
uniform float u_terrainTextureScale;
uniform int u_terrainDebugMode;
uniform sampler2D vegetationDensityMap;
uniform bool hasVegetationDensity;
uniform float vegetationTerrainSize;

#include "../common/terrain_biomes.glsl"

vec2 parallaxMapping(vec2 uv, vec3 viewDir)
{
    float layerDepth = 1.0 / float(numLayers);
    vec2 delta = viewDir.xy / max(viewDir.z, 0.05) *
                 parallaxHeightScale / float(numLayers);
    float depth = 0.0;
    vec2 current = uv;
    float sampleDepth = texture(parallaxTexture, current).r;
    while (depth < sampleDepth)
    {
        current -= delta;
        sampleDepth = texture(parallaxTexture, current).r;
        depth += layerDepth;
    }
    vec2 previous = current + delta;
    float afterDepth = sampleDepth - depth;
    float beforeDepth = texture(parallaxTexture, previous).r -
                        (depth - layerDepth);
    float amount = afterDepth / max(afterDepth - beforeDepth, 1e-5);
    return mix(current, previous, clamp(amount, 0.0, 1.0));
}

vec3 triplanarWeights(vec3 n)
{
    vec3 weights = pow(abs(n), vec3(4.0));
    return weights / max(dot(weights, vec3(1.0)), 1e-5);
}

vec3 triplanarColor(sampler2D map, vec3 position, vec3 weights, float scale)
{
    vec3 x = texture(map, position.zy * scale).rgb;
    vec3 y = texture(map, position.xz * scale).rgb;
    vec3 z = texture(map, position.xy * scale).rgb;
    return x * weights.x + y * weights.y + z * weights.z;
}

float triplanarScalar(sampler2D map, vec3 position, vec3 weights, float scale)
{
    return texture(map, position.zy * scale).r * weights.x +
           texture(map, position.xz * scale).r * weights.y +
           texture(map, position.xy * scale).r * weights.z;
}

vec3 triplanarNormal(sampler2D map, vec3 position, vec3 baseNormal,
                     vec3 weights, float scale)
{
    vec3 nx = texture(map, position.zy * scale).xyz * 2.0 - 1.0;
    vec3 ny = texture(map, position.xz * scale).xyz * 2.0 - 1.0;
    vec3 nz = texture(map, position.xy * scale).xyz * 2.0 - 1.0;
    nx = vec3(nx.z * sign(baseNormal.x), nx.y, nx.x);
    ny = vec3(ny.x, ny.z * sign(baseNormal.y), ny.y);
    nz = vec3(nz.x, nz.y, nz.z * sign(baseNormal.z));
    return normalize(nx * weights.x + ny * weights.y + nz * weights.z);
}

vec3 pseudoColor(float value)
{
    value = clamp(value, 0.0, 1.0);
    return clamp(vec3(1.5 - abs(4.0 * value - 3.0),
                      1.5 - abs(4.0 * value - 2.0),
                      1.5 - abs(4.0 * value - 1.0)), 0.0, 1.0);
}

vec3 rotateHue(vec3 color, float angle)
{
    const vec3 axis = vec3(0.57735026919);
    return color * cos(angle) + cross(axis, color) * sin(angle) +
           axis * dot(axis, color) * (1.0 - cos(angle));
}

vec3 heightBlend(vec3 biome, vec3 materialHeight)
{
    // Height detail only shapes the two authored transition bands. Materials
    // whose biome weight is zero remain exactly zero, so pure rock and pure
    // snow regions cannot be contaminated by another texture.
    vec3 score = biome + materialHeight * u_terrainBlendSharpness;
    float maximum = max(score.x, max(score.y, score.z));
    vec3 result = exp((score - maximum) * 8.0) * step(vec3(0.0001), biome);
    return result / max(dot(result, vec3(1.0)), 1e-4);
}

void main()
{
    vec2 texCoords = TexCoords;
    if (!useTerrainBlend && enableParallaxMapping && hasParallaxMap)
    {
        vec3 viewDir = normalize(TangentViewPos - TangentFragPos);
        texCoords = parallaxMapping(texCoords, viewDir);
    }
    if (alphaMask && hasAlbedoMap &&
        texture(albedoTexture, texCoords).a < alphaCutoff)
        discard;

    vec3 normal = normalize(Normal);
    vec3 albedo = pow(albedoColor, vec3(2.2));
    float roughness = roughnessFactor;
    float metallic = metallicFactor;

    if (useTerrainBlend && hasTerrainData)
    {
        vec4 data = texture(terrainDataMap, TexCoords1);
        // 20-50 m snow-line breakup plus a lower frequency macro signal.
        float macroNoise = texture(terrainNoiseTexture, FragPos.xz / 480.0).r;
        float edgeNoise = texture(terrainNoiseTexture, FragPos.xz / 34.0).g;
        float biomeNoise = mix(macroNoise, edgeNoise, 0.65);
        vec3 biome = biomeWeights(data.r, data.b, biomeNoise);
        vec2 environment = hasTerrainEnvironment
            ? texture(terrainEnvironmentMap, TexCoords1).rg
            : vec2(0.5, biome.z);
        // Material ownership is strictly height-banded. Slope, moisture and
        // retained-snow fields remain available as debug/environment data but
        // no longer replace a low grass or middle rock region.
        float scale = u_terrainTextureScale;
        vec3 planeWeights = triplanarWeights(normal);
        float rockHeight = triplanarScalar(
            terrainRockHeight, FragPos, planeWeights, scale);
        float grassHeight = texture(terrainGrassHeight,
                                    FragPos.xz * scale).r;
        float snowHeight = texture(terrainSnowHeight,
                                   FragPos.xz * scale * 0.82).r;
        vec3 materialBiome = vec3(biome.y, biome.x, biome.z);
        vec3 weights = heightBlend(materialBiome,
            vec3(rockHeight, grassHeight, snowHeight));

        vec3 rockAlbedo = triplanarColor(
            albedoTexture, FragPos, planeWeights, scale);
        vec3 grassAlbedo = texture(
            terrainGrassAlbedo, FragPos.xz * scale * 0.72).rgb;
        vec3 snowAlbedo = texture(
            terrainSnowAlbedo, FragPos.xz * scale * 0.82).rgb;
        // PBR textures are sRGB albedo sources; macro variation remains in
        // linear space and avoids flattening their real surface detail.
        rockAlbedo = pow(rockAlbedo, vec3(2.2));
        grassAlbedo = pow(grassAlbedo, vec3(2.2));
        snowAlbedo = pow(min(snowAlbedo, vec3(0.92)), vec3(2.2));
        float brightness = mix(0.90, 1.10, macroNoise);
        grassAlbedo = rotateHue(grassAlbedo,
                                (macroNoise - 0.5) * 0.27925268) * brightness;
        rockAlbedo *= mix(0.88, 1.08, macroNoise);
        float sunFacing = cos(data.b * TERRAIN_TWO_PI - u_sunAzimuth);
        snowAlbedo *= mix(vec3(0.92, 0.96, 1.0), vec3(1.0),
                          sunFacing * 0.5 + 0.5);
        albedo = rockAlbedo * weights.x +
                 grassAlbedo * weights.y + snowAlbedo * weights.z;
        if (hasVegetationDensity)
        {
            vec2 densityUV = FragPos.xz / vegetationTerrainSize + 0.5;
            float vegetationDensity = texture(vegetationDensityMap,
                                              clamp(densityUV, 0.0, 1.0)).r;
            albedo *= mix(1.0, 0.90, vegetationDensity * weights.y);
        }

        if (enableNormalMapping)
        {
            vec3 rockNormal = hasNormalMap
                ? triplanarNormal(normalTexture, FragPos, normal,
                                  planeWeights, scale)
                : normal;
            vec3 grassTangent = hasTerrainGrassNormal
                ? texture(terrainGrassNormal,
                          FragPos.xz * scale * 0.72).xyz * 2.0 - 1.0
                : vec3(0.0, 0.0, 1.0);
            vec3 snowTangent = hasTerrainSnowNormal
                ? texture(terrainSnowNormal,
                          FragPos.xz * scale * 0.82).xyz * 2.0 - 1.0
                : vec3(0.0, 0.0, 1.0);
            vec3 grassNormal = normalize(TBN * grassTangent);
            vec3 snowNormal = normalize(TBN * snowTangent);
            // Material-specific detail strength: rock > grass > snow.
            rockNormal = normalize(mix(normal, rockNormal,
                                       min(bumpNormalStrength * 1.15, 1.0)));
            grassNormal = normalize(mix(normal, grassNormal,
                                        min(bumpNormalStrength * 0.72, 1.0)));
            snowNormal = normalize(mix(normal, snowNormal,
                                       min(bumpNormalStrength * 0.32, 1.0)));
            normal = normalize(rockNormal * weights.x +
                               grassNormal * weights.y +
                               snowNormal * weights.z);
            vec2 packedDetail = texture(
                terrainNoiseTexture, FragPos.xz / 18.0).ba * 2.0 - 1.0;
            vec3 detailNormal = normalize(vec3(
                packedDetail.x,
                sqrt(max(1.0 - dot(packedDetail, packedDetail), 0.05)),
                packedDetail.y));
            float detailStrength = dot(weights, vec3(0.72, 0.48, 0.18));
            normal = normalize(mix(normal, detailNormal,
                                   detailStrength * 0.24));
        }

        float rockRoughness = hasRoughnessMap
            ? triplanarScalar(roughnessTexture, FragPos,
                              planeWeights, scale) : 0.78;
        float grassRoughness = hasTerrainGrassRoughness
            ? texture(terrainGrassRoughness,
                      FragPos.xz * scale * 0.72).r : 0.90;
        float snowRoughness = hasTerrainSnowRoughness
            ? texture(terrainSnowRoughness,
                      FragPos.xz * scale * 0.82).r : 0.42;
        roughness = dot(vec3(rockRoughness, grassRoughness,
                             snowRoughness), weights);
        metallic = 0.0;

        if (u_terrainDebugMode > 0)
        {
            float debugValue = u_terrainDebugMode <= 4
                ? data[u_terrainDebugMode - 1]
                : environment[u_terrainDebugMode - 5];
            if (u_terrainDebugMode == 3 && data.g < 0.001)
                albedo = vec3(0.5); // flat: no orientation
            else
                albedo = pseudoColor(debugValue);
            normal = normalize(Normal);
            roughness = 1.0;
        }
    }
    else
    {
        if (hasAlbedoMap)
            albedo = pow(texture(albedoTexture, texCoords).rgb, vec3(2.2));
        if (enableNormalMapping && hasNormalMap)
        {
            vec3 mapped = texture(normalTexture, texCoords).rgb * 2.0 - 1.0;
            mapped = normalize(mix(vec3(0.0, 0.0, 1.0), mapped,
                                   bumpNormalStrength));
            normal = normalize(TBN * mapped);
        }
        if (usePackedMetallicRoughness)
        {
            vec3 mr = texture(metallicTexture, texCoords).rgb;
            roughness = mr.g;
            metallic = mr.b;
        }
        else
        {
            if (hasRoughnessMap)
                roughness = texture(roughnessTexture, texCoords).r;
            if (hasMetallicMap)
                metallic = texture(metallicTexture, texCoords).r;
        }
    }

    vec3 dx = dFdx(normal);
    vec3 dy = dFdy(normal);
    float variance = 0.5 * (dot(dx, dx) + dot(dy, dy));
    roughness = sqrt(clamp(roughness * roughness + min(variance, 0.32),
                           0.0016, 1.0));
    gPosition = FragPos;
    gNormalRoughness = vec4(normal, clamp(roughness, 0.04, 1.0));
    gAlbedoMetallic = vec4(albedo, clamp(metallic, 0.0, 1.0));
    gVelocity = vec2(0.0);
    gCoverageReactive = vec2(0.0);
}
