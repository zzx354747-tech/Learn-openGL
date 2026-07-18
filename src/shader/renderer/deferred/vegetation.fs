#version 330 core

layout(location=0) out vec3 gPosition;
layout(location=1) out vec4 gNormalRoughness;
layout(location=2) out vec4 gAlbedoMetallic;
layout(location=3) out vec2 gVelocity;
layout(location=4) out vec2 gCoverageReactive;

in vec3 vWorldPosition;
in vec3 vNormal;
in vec4 vColorRoughness;
in vec2 vVelocity;
in vec4 vUVMaterial;
flat in int vPointMode;
flat in float vInstanceHash;
flat in float vRepresentationCoverage;

uniform sampler2D u_baseColorTexture0;
uniform sampler2D u_baseColorTexture1;
uniform sampler2D u_baseColorTexture2;
uniform sampler2D u_baseColorTexture3;
uniform sampler2D u_normalTexture0;
uniform sampler2D u_normalTexture1;
uniform sampler2D u_normalTexture2;
uniform sampler2D u_normalTexture3;
uniform sampler2D u_baseColorAtlas;
uniform sampler2D u_normalAtlas;
uniform sampler2D u_foliageDataAtlas;
uniform vec4 u_materialTileRects[4];
uniform float u_materialAlphaCutoffs[4];
uniform int u_materialFlags[4];
uniform bool u_hasMaterialAtlas;
uniform float u_materialMipScale;
uniform bool u_hasBaseColorTexture;
uniform bool u_hasNormalTexture;
uniform bool u_alphaMask;
uniform float u_alphaCutoff;
uniform int u_pointShape;
uniform vec3 u_speciesTint;
uniform float u_speciesSaturation;
uniform sampler2D u_blueNoiseTexture;
uniform bool u_hasBlueNoiseTexture;
uniform bool u_taaEnabled;
uniform int u_frameIndex;
uniform float u_lodCoverage;
uniform bool u_lodFadeIn;
uniform vec3 u_cameraPosition;

float blueNoise()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 offset = u_taaEnabled
        ? ivec2((u_frameIndex * 23) & 63,
                (u_frameIndex * 47) & 63) : ivec2(0);
    if (u_hasBlueNoiseTexture)
        return texelFetch(u_blueNoiseTexture,
                          (pixel + offset) & ivec2(63), 0).r;
    return fract(sin(dot(vec2(pixel + offset), vec2(12.9898, 78.233)))
                 * 43758.5453);
}

float alphaCoverage(float alpha, float cutoff)
{
    float width = max(fwidth(alpha), 1.0 / 255.0);
    return smoothstep(cutoff - width, cutoff + width, alpha);
}

vec4 sampleBaseColor(int index, vec2 uv)
{
    if (index == 1) return texture(u_baseColorTexture0, uv);
    if (index == 2) return texture(u_baseColorTexture1, uv);
    if (index == 3) return texture(u_baseColorTexture2, uv);
    if (index == 4) return texture(u_baseColorTexture3, uv);
    return vec4(1.0);
}

vec3 sampleNormalMap(int index, vec2 uv)
{
    if (index == 1) return texture(u_normalTexture0, uv).xyz;
    if (index == 2) return texture(u_normalTexture1, uv).xyz;
    if (index == 3) return texture(u_normalTexture2, uv).xyz;
    if (index == 4) return texture(u_normalTexture3, uv).xyz;
    return vec3(0.5, 0.5, 1.0);
}

vec2 atlasUV(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    return tile.xy + fract(uv) * tile.zw;
}

vec4 sampleAtlasBase(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    return textureGrad(u_baseColorAtlas, atlasUV(index, uv),
                       dFdx(uv) * tile.zw * u_materialMipScale,
                       dFdy(uv) * tile.zw * u_materialMipScale);
}

vec3 sampleAtlasNormal(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    return textureGrad(u_normalAtlas, atlasUV(index, uv),
                       dFdx(uv) * tile.zw * u_materialMipScale,
                       dFdy(uv) * tile.zw * u_materialMipScale).xyz;
}

vec4 sampleAtlasData(int index, vec2 uv)
{
    vec4 tile = u_materialTileRects[index];
    return textureGrad(u_foliageDataAtlas, atlasUV(index, uv),
                       dFdx(uv) * tile.zw * u_materialMipScale,
                       dFdy(uv) * tile.zw * u_materialMipScale);
}

mat3 cotangentFrame(vec3 normal, vec3 position, vec2 uv)
{
    vec3 dp1 = dFdx(position);
    vec3 dp2 = dFdy(position);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2Perp = cross(dp2, normal);
    vec3 dp1Perp = cross(normal, dp1);
    vec3 tangent = dp2Perp * duv1.x + dp1Perp * duv2.x;
    vec3 bitangent = dp2Perp * duv1.y + dp1Perp * duv2.y;
    float scale = inversesqrt(max(max(dot(tangent, tangent),
                                     dot(bitangent, bitangent)), 1e-8));
    return mat3(tangent * scale, bitangent * scale, normal);
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
        bool trunk = y <= 0.24 && abs(q.x) < (shape == 1 ? 0.09 : 0.065);
        return !(crown || trunk);
    }
    if (shape <= 4)
    {
        vec2 shrub = vec2(q.x * (shape == 4 ? 0.72 : 0.92),
                          (q.y + 0.28) * 1.18);
        return dot(shrub, shrub) > 0.92;
    }
    if (shape <= 7)
    {
        float bladeA = abs(q.x - 0.22 * sin(y * 7.0));
        float bladeB = abs(q.x + 0.34 - 0.12 * sin(y * 9.0));
        float bladeC = abs(q.x - 0.38 + 0.10 * sin(y * 8.0));
        float width = mix(0.09, 0.018, y);
        bool blades = y < 0.78 && min(bladeA, min(bladeB, bladeC)) < width;
        bool base = y < 0.20 && abs(q.x) < 0.72 * (1.0 - y * 2.2);
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
    float representationCoverage =
        clamp(vRepresentationCoverage, 0.0, 1.0);
    if (vPointMode == 0)
    {
        if (vInstanceHash > representationCoverage)
            discard;
    }
    else if (vInstanceHash < 1.0 - representationCoverage)
        discard;
    float temporalCoverage = clamp(u_lodCoverage, 0.0, 1.0);
    float analyticCoverage = clamp(vRepresentationCoverage, 0.0, 1.0) *
                             temporalCoverage;
    float coverageNoise = blueNoise();

    if (vPointMode != 0)
    {
        vec2 q = gl_PointCoord * 2.0 - 1.0;
        if (outsidePointSilhouette(q, u_pointShape))
            discard;
        // Procedural point silhouettes have no alpha texture gradient. Mark
        // them mildly reactive so TAA does not sharpen their one-pixel edges.
        analyticCoverage *= 0.70;
    }

    vec3 n = normalize(vNormal);
    if (!gl_FrontFacing)
        n = -n;
    vec4 baseSample = vec4(1.0);
    vec4 materialData = vec4(1.0, vColorRoughness.a, 0.0, 1.0);
    // Every non-point fragment comes from the procedural vegetation factory.
    bool foliage = vPointMode == 0;
    if (vPointMode == 0 && u_hasMaterialAtlas && vUVMaterial.z > 0.5)
    {
        int materialIndex = clamp(int(vUVMaterial.z + 0.5) - 1, 0, 3);
        baseSample = sampleAtlasBase(materialIndex, vUVMaterial.xy);
        materialData = sampleAtlasData(materialIndex, vUVMaterial.xy);
        int flags = u_materialFlags[materialIndex];
        if ((flags & 1) != 0)
        {
            float coverage = alphaCoverage(
                baseSample.a, u_materialAlphaCutoffs[materialIndex]);
            temporalCoverage *= coverage;
            analyticCoverage *= coverage;
        }
        vec3 mapped = sampleAtlasNormal(
            materialIndex, vUVMaterial.xy) * 2.0 - 1.0;
        float normalStrength = (1.0 - smoothstep(
            80.0, 300.0, distance(vWorldPosition, u_cameraPosition))) *
            clamp(materialData.a, 0.0, 1.0);
        mapped = normalize(mix(vec3(0.0, 0.0, 1.0),
                               mapped, normalStrength));
        n = normalize(cotangentFrame(
            n, vWorldPosition, vUVMaterial.xy) * mapped);
        foliage = foliage || (flags & 2) != 0;
    }
    else
    {
        if (vPointMode == 0 && u_hasBaseColorTexture &&
            vUVMaterial.z > 0.5)
            baseSample = sampleBaseColor(
                int(vUVMaterial.z + 0.5), vUVMaterial.xy);
        if (vPointMode == 0 && u_alphaMask && vUVMaterial.z > 0.5)
        {
            float coverage = alphaCoverage(baseSample.a, u_alphaCutoff);
            temporalCoverage *= coverage;
            analyticCoverage *= coverage;
        }
        if (vPointMode == 0 && u_hasNormalTexture &&
            vUVMaterial.w > 0.5)
        {
            vec3 mapped = sampleNormalMap(
                int(vUVMaterial.w + 0.5), vUVMaterial.xy) * 2.0 - 1.0;
            float normalStrength = 1.0 - smoothstep(
                80.0, 300.0, distance(vWorldPosition, u_cameraPosition));
            mapped = normalize(mix(vec3(0.0, 0.0, 1.0),
                                   mapped, normalStrength));
            n = normalize(cotangentFrame(
                n, vWorldPosition, vUVMaterial.xy) * mapped);
        }
    }
    bool proceduralOpaque =
        vPointMode == 0 && !u_hasMaterialAtlas &&
        !u_hasBaseColorTexture && !u_alphaMask;
    if (proceduralOpaque)
    {
        // Select one whole-instance LOD with a complementary stable hash.
        // Pixel-level temporal dithering turns sub-pixel procedural blades
        // into moving noise even when TAA is enabled.
        if (u_lodFadeIn)
        {
            if (vInstanceHash < 1.0 - temporalCoverage)
                discard;
        }
        else if (vInstanceHash > temporalCoverage)
            discard;
    }
    else if (coverageNoise > temporalCoverage)
        discard;
    vec3 dx = dFdx(n), dy = dFdy(n);
    float variance = .5 * (dot(dx,dx) + dot(dy,dy));
    float authoredRoughness = clamp(materialData.g, 0.04, 1.0);
    float roughness = sqrt(clamp(authoredRoughness * authoredRoughness +
                           min(variance,.24), .0016, 1));
    gPosition = vWorldPosition;
    // A negative roughness marks vegetation without allocating another
    // G-buffer attachment. Non-vegetation geometry always writes positive
    // roughness, so the encoding is backward compatible.
    gNormalRoughness = vec4(
        n, foliage ? -clamp(roughness,.04,1) : clamp(roughness,.04,1));
    vec3 albedo = vColorRoughness.rgb * baseSample.rgb *
                  mix(1.0, materialData.r, 0.35);
    float luminance = dot(albedo, vec3(0.2126, 0.7152, 0.0722));
    albedo = mix(vec3(luminance), albedo, u_speciesSaturation) *
             u_speciesTint;
    // Foliage is non-metallic; its otherwise-unused metallic channel carries
    // the baked thickness/transmission proxy for the deferred light pass.
    gAlbedoMetallic = vec4(
        max(albedo, vec3(0.0)),
        foliage ? clamp(materialData.b, 0.0, 1.0) : 0.0);
    gVelocity = vVelocity;
    // A single-sample G-buffer cannot reconstruct the geometric sample
    // coverage missed outside a thin procedural triangle. Conservatively tag
    // every surviving procedural-opaque fragment so the existing TAA resolve
    // retains more history and suppresses sharpening for this material. Alpha
    // cards keep their analytic alpha/LOD coverage unchanged.
    float taaCoverage = proceduralOpaque ? 0.0 : analyticCoverage;
    gCoverageReactive = vec2(clamp(taaCoverage, 0.0, 1.0), 1.0);
}
