#version 330 core

layout(location=0) out vec4 FragColor;
layout(location=1) out vec4 BrightColor;

in vec4 vColorRoughness;
in vec4 vUVMaterial;
flat in int vPointMode;

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
uniform int u_pointShape;
uniform vec3 u_speciesTint;
uniform float u_speciesSaturation;
uniform float u_vegetationExposure;

vec4 sampleBaseColor(int index, vec2 uv)
{
    if (index == 1) return texture(u_baseColorTexture0, uv);
    if (index == 2) return texture(u_baseColorTexture1, uv);
    if (index == 3) return texture(u_baseColorTexture2, uv);
    if (index == 4) return texture(u_baseColorTexture3, uv);
    return vec4(1.0);
}

vec2 atlasUV(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    return tile.xy + fract(uv) * tile.zw;
}

vec4 sampleAtlasBase(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    return textureGrad(
        u_baseColorAtlas,
        atlasUV(index, uv),
        dFdx(uv) * tile.zw * u_materialMipScale,
        dFdy(uv) * tile.zw * u_materialMipScale);
}

bool outsidePointSilhouette(vec2 q, int shape)
{
    float y = q.y * 0.5 + 0.5;
    if (shape <= 2)
    {
        float asymmetry = shape == 0 ? 0.05 : (shape == 1 ? -0.10 : 0.12);
        float halfWidth = mix(0.88, 0.025, pow(y, shape == 1 ? 0.72 : 0.92));
        halfWidth *= 0.90 + 0.10 * sin(y * 29.0 + float(shape) * 2.1);
        float shiftedX = q.x - asymmetry * (y - 0.35);
        bool crown = y > 0.08 && abs(shiftedX) < halfWidth;
        bool trunk = y <= 0.24 &&
                     abs(q.x) < (shape == 1 ? 0.09 : 0.065);
        return !(crown || trunk);
    }
    if (shape <= 4)
    {
        vec2 shrub = vec2(
            q.x * (shape == 4 ? 0.72 : 0.92),
            (q.y + 0.28) * 1.18);
        return dot(shrub, shrub) > 0.92;
    }
    if (shape <= 7)
    {
        float bladeA = abs(q.x - 0.22 * sin(y * 7.0));
        float bladeB = abs(q.x + 0.34 - 0.12 * sin(y * 9.0));
        float bladeC = abs(q.x - 0.38 + 0.10 * sin(y * 8.0));
        float width = mix(0.09, 0.018, y);
        bool blades =
            y < 0.78 && min(bladeA, min(bladeB, bladeC)) < width;
        bool base =
            y < 0.20 && abs(q.x) < 0.72 * (1.0 - y * 2.2);
        return !(blades || base);
    }
    if (shape <= 12)
    {
        vec2 flower = vec2(q.x, q.y + 0.32);
        bool head = dot(flower, flower) < 0.20;
        bool stem = y < 0.48 && abs(q.x) < 0.055;
        return !(head || stem);
    }
    vec2 cushion = vec2(q.x * 0.82, (q.y + 0.55) * 1.65);
    return dot(cushion, cushion) > 0.88;
}

void main()
{
    if (vPointMode != 0)
    {
        vec2 q = gl_PointCoord * 2.0 - 1.0;
        if (outsidePointSilhouette(q, u_pointShape))
            discard;
    }

    vec4 baseSample = vec4(1.0);
    if (vPointMode == 0 && u_hasMaterialAtlas &&
        vUVMaterial.z > 0.5)
    {
        int materialIndex =
            clamp(int(vUVMaterial.z + 0.5) - 1, 0, 3);
        baseSample =
            sampleAtlasBase(materialIndex, vUVMaterial.xy);
        int flags = u_materialFlags[materialIndex];
        if ((flags & 1) != 0 &&
            baseSample.a < u_materialAlphaCutoffs[materialIndex])
            discard;
    }
    else
    {
        if (vPointMode == 0 && u_hasBaseColorTexture &&
            vUVMaterial.z > 0.5)
        {
            baseSample = sampleBaseColor(
                int(vUVMaterial.z + 0.5), vUVMaterial.xy);
        }
        if (vPointMode == 0 && u_alphaMask &&
            vUVMaterial.z > 0.5 && baseSample.a < u_alphaCutoff)
            discard;
    }

    vec3 albedo = vColorRoughness.rgb * baseSample.rgb;
    float luminance =
        dot(albedo, vec3(0.2126, 0.7152, 0.0722));
    albedo =
        mix(vec3(luminance), albedo, u_speciesSaturation) *
        u_speciesTint;
    // Far point-sprite LODs intentionally keep their original value and do
    // not participate in the dynamic geometric-vegetation exposure path.
    float exposure = vPointMode != 0 ? 1.0 : u_vegetationExposure;
    FragColor = vec4(max(albedo * exposure, vec3(0.0)), 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
