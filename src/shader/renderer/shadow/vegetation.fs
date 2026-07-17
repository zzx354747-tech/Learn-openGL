#version 330 core
in vec4 vUVMaterial;
uniform sampler2D u_baseColorTexture0;
uniform sampler2D u_baseColorTexture1;
uniform sampler2D u_baseColorTexture2;
uniform sampler2D u_baseColorTexture3;
uniform sampler2D u_baseColorAtlas;
uniform vec4 u_materialTileRects[4];
uniform float u_materialAlphaCutoffs[4];
uniform int u_materialFlags[4];
uniform bool u_hasMaterialAtlas;
uniform float u_materialMipScale;
uniform bool u_hasBaseColorTexture;
uniform bool u_alphaMask;
uniform float u_alphaCutoff;
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
void main()
{
    if (u_hasMaterialAtlas && vUVMaterial.z > 0.5)
    {
        int materialIndex = clamp(int(vUVMaterial.z + 0.5) - 1, 0, 3);
        if ((u_materialFlags[materialIndex] & 1) != 0 &&
            sampleAtlasAlpha(materialIndex, vUVMaterial.xy) <
                u_materialAlphaCutoffs[materialIndex])
            discard;
        return;
    }
    if (u_hasBaseColorTexture && u_alphaMask && vUVMaterial.z > 0.5 &&
        sampleAlpha(int(vUVMaterial.z + 0.5), vUVMaterial.xy) < u_alphaCutoff)
        discard;
}
