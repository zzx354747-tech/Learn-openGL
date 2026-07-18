#version 330 core

in vec4 vUVMaterial;
in vec3 vWorldPosition;
flat in float vInstanceHash;

uniform sampler2D u_baseColorTexture0;
uniform sampler2D u_baseColorTexture1;
uniform sampler2D u_baseColorTexture2;
uniform sampler2D u_baseColorTexture3;
uniform sampler2D u_baseColorAtlas;
uniform sampler2D u_blueNoiseTexture;
uniform vec4 u_materialTileRects[4];
uniform float u_materialAlphaCutoffs[4];
uniform int u_materialFlags[4];
uniform bool u_hasMaterialAtlas;
uniform float u_materialMipScale;
uniform bool u_hasBaseColorTexture;
uniform bool u_alphaMask;
uniform float u_alphaCutoff;
uniform bool u_hasBlueNoiseTexture;
uniform vec3 u_cameraPosition;
uniform float u_shadowFadeStart;
uniform float u_shadowFadeEnd;

float sampleAlpha(int index, vec2 uv)
{
    if (index == 1) return texture(u_baseColorTexture0, uv).a;
    if (index == 2) return texture(u_baseColorTexture1, uv).a;
    if (index == 3) return texture(u_baseColorTexture2, uv).a;
    if (index == 4) return texture(u_baseColorTexture3, uv).a;
    return 1.0;
}

float sampleAtlasAlpha(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    vec2 atlasUv = tile.xy + fract(uv) * tile.zw;
    return textureGrad(u_baseColorAtlas, atlasUv,
                       dFdx(uv) * tile.zw * u_materialMipScale,
                       dFdy(uv) * tile.zw * u_materialMipScale).a;
}

float coverage(float alpha, float cutoff)
{
    float width = max(fwidth(alpha), 1.0 / 255.0);
    return smoothstep(cutoff - width, cutoff + width, alpha);
}

float stableNoise()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy) & ivec2(63);
    if (u_hasBlueNoiseTexture)
        return texelFetch(u_blueNoiseTexture, pixel, 0).r;
    return fract(sin(dot(vec2(pixel), vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    float distanceXZ = length(vWorldPosition.xz - u_cameraPosition.xz);
    float shadowCoverage = 1.0 - smoothstep(
        u_shadowFadeStart, u_shadowFadeEnd, distanceXZ);
    // Instance-stable fade avoids a moving screen-space stipple in the map.
    if (vInstanceHash > shadowCoverage)
        discard;

    float alpha = 1.0;
    float cutoff = u_alphaCutoff;
    bool masked = false;
    if (u_hasMaterialAtlas && vUVMaterial.z > 0.5)
    {
        int materialIndex = clamp(int(vUVMaterial.z + 0.5) - 1, 0, 3);
        masked = (u_materialFlags[materialIndex] & 1) != 0;
        if (masked)
        {
            alpha = sampleAtlasAlpha(materialIndex, vUVMaterial.xy);
            cutoff = u_materialAlphaCutoffs[materialIndex];
        }
    }
    else if (u_hasBaseColorTexture && u_alphaMask && vUVMaterial.z > 0.5)
    {
        masked = true;
        alpha = sampleAlpha(int(vUVMaterial.z + 0.5), vUVMaterial.xy);
    }
    if (masked && stableNoise() > coverage(alpha, cutoff))
        discard;
}
