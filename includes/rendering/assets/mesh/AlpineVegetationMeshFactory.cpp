#include "rendering/assets/mesh/AlpineVegetationMeshFactory.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace
{
constexpr float Pi = 3.14159265358979323846f;

struct Random
{
    std::uint32_t state;
    float unit()
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8u) & 0x00ffffffu) / 16777215.0f;
    }
    float range(float a, float b) { return glm::mix(a, b, unit()); }
};

void vertex(VegetationMeshData& mesh, const glm::vec3& p,
            const glm::vec3& n, const glm::vec3& color, float roughness,
            float wind, float variation)
{
    mesh.vertices.push_back({p, n, glm::vec4(color, roughness),
                             glm::vec2(wind, variation)});
}

void triangle(VegetationMeshData& mesh, const glm::vec3& a,
              const glm::vec3& b, const glm::vec3& c,
              const glm::vec3& color, float roughness, float wind,
              float variation, float upwardBias = 0.0f)
{
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    if (n.y < -0.92f && upwardBias > 0.0f)
        n = glm::normalize(n + glm::vec3(0.0f, upwardBias * 0.35f, 0.0f));
    else if (upwardBias > 0.0f)
        n = glm::normalize(n * (1.0f - upwardBias) +
                           glm::vec3(0.0f, 1.0f, 0.0f) * upwardBias);
    const std::uint16_t first = static_cast<std::uint16_t>(mesh.vertices.size());
    vertex(mesh, a, n, color, roughness, wind, variation);
    vertex(mesh, b, n, color, roughness, wind, variation);
    vertex(mesh, c, n, color, roughness, wind, variation);
    mesh.indices.insert(mesh.indices.end(), {first, static_cast<std::uint16_t>(first + 1u),
                                             static_cast<std::uint16_t>(first + 2u)});
}

void quad(VegetationMeshData& mesh, const glm::vec3& a, const glm::vec3& b,
          const glm::vec3& c, const glm::vec3& d, const glm::vec3& color,
          float roughness, float wind, float variation, float upwardBias = 0.0f)
{
    triangle(mesh, a, b, c, color, roughness, wind, variation, upwardBias);
    triangle(mesh, a, c, d, color, roughness, wind, variation, upwardBias);
}

void appendTaperedPrism(VegetationMeshData& mesh, float height, int sides)
{
    constexpr std::array<float, 5> heights = {-0.05f, 0.0f, 0.40f, 0.75f, 0.98f};
    constexpr std::array<float, 5> radii = {0.045f, 0.036f, 0.025f, 0.014f, 0.004f};
    const glm::vec3 trunk(0.16f, 0.075f, 0.025f);
    for (int ring = 0; ring < 4; ++ring)
    {
        for (int i = 0; i < sides; ++i)
        {
            const float a0 = 2.0f * Pi * static_cast<float>(i) / sides;
            const float a1 = 2.0f * Pi * static_cast<float>(i + 1) / sides;
            const glm::vec3 p0(std::cos(a0) * radii[ring] * height,
                               heights[ring] * height,
                               std::sin(a0) * radii[ring] * height);
            const glm::vec3 p1(std::cos(a1) * radii[ring] * height,
                               heights[ring] * height,
                               std::sin(a1) * radii[ring] * height);
            const glm::vec3 p2(std::cos(a1) * radii[ring + 1] * height,
                               heights[ring + 1] * height,
                               std::sin(a1) * radii[ring + 1] * height);
            const glm::vec3 p3(std::cos(a0) * radii[ring + 1] * height,
                               heights[ring + 1] * height,
                               std::sin(a0) * radii[ring + 1] * height);
            quad(mesh, p0, p1, p2, p3, trunk, 0.92f, 0.0f, 0.15f);
        }
    }
}

void appendFoliageTier(VegetationMeshData& mesh, float centerY, float radius,
                       float thickness, int segments, float angularOffset,
                       std::uint32_t seed, float treeHeight)
{
    Random rng{seed};
    const float phase = rng.range(0.0f, 2.0f * Pi);
    const int notchA = static_cast<int>(rng.unit() * segments) % segments;
    const int notchB = (notchA + 2 + static_cast<int>(rng.unit() * (segments - 3))) % segments;
    std::vector<float> irregular(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i)
    {
        const float angle = angularOffset + 2.0f * Pi * static_cast<float>(i) / segments;
        float r = 1.0f + 0.12f * std::sin(angle * 3.0f + phase) + rng.range(-0.08f, 0.08f);
        if (i == notchA || i == notchB)
            r *= rng.range(0.70f, 0.88f);
        irregular[static_cast<std::size_t>(i)] = r;
    }
    const glm::vec3 dark(0.025f, 0.070f, 0.018f);
    const glm::vec3 base(0.045f, 0.120f, 0.030f);
    const glm::vec3 tip(0.070f, 0.165f, 0.040f);
    const float normalizedHeight = glm::clamp(centerY / treeHeight, 0.0f, 1.0f);
    const glm::vec3 upperColor = glm::mix(base, tip, normalizedHeight * 0.72f);
    for (int i = 0; i < segments; ++i)
    {
        const int j = (i + 1) % segments;
        const float a0 = angularOffset + 2.0f * Pi * static_cast<float>(i) / segments;
        const float a1 = angularOffset + 2.0f * Pi * static_cast<float>(j) / segments;
        const auto radial = [](float a) { return glm::vec3(std::cos(a), 0.0f, std::sin(a)); };
        const glm::vec3 top0 = radial(a0) * radius * irregular[i] * 0.24f +
                               glm::vec3(0.0f, centerY + thickness * 0.42f, 0.0f);
        const glm::vec3 top1 = radial(a1) * radius * irregular[j] * 0.24f +
                               glm::vec3(0.0f, centerY + thickness * 0.42f, 0.0f);
        const glm::vec3 outer0 = radial(a0) * radius * irregular[i] +
                                 glm::vec3(0.0f, centerY, 0.0f);
        const glm::vec3 outer1 = radial(a1) * radius * irregular[j] +
                                 glm::vec3(0.0f, centerY, 0.0f);
        const glm::vec3 lower0 = radial(a0) * radius * irregular[i] * 0.56f +
                                 glm::vec3(0.0f, centerY - thickness * 0.58f, 0.0f);
        const glm::vec3 lower1 = radial(a1) * radius * irregular[j] * 0.56f +
                                 glm::vec3(0.0f, centerY - thickness * 0.58f, 0.0f);
        quad(mesh, top0, outer0, outer1, top1, upperColor, 0.86f,
             0.15f + normalizedHeight * 0.12f, 1.0f, 0.28f);
        quad(mesh, outer0, lower0, lower1, outer1, dark, 0.92f,
             0.12f + normalizedHeight * 0.10f, 1.0f, 0.22f);
    }
}

void appendGroundedIrregularCone(VegetationMeshData& mesh, float height,
                                 float radius, int segments,
                                 std::uint32_t seed)
{
    Random rng{seed};
    const float phase = rng.range(0.0f, 2.0f * Pi);
    const glm::vec3 dark(0.025f, 0.070f, 0.018f);
    const glm::vec3 base(0.045f, 0.120f, 0.030f);
    const glm::vec3 tip(0.070f, 0.165f, 0.040f);
    std::vector<float> irregular(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i)
    {
        const float angle = 2.0f * Pi * static_cast<float>(i) / segments;
        irregular[i] = 1.0f + 0.12f * std::sin(angle * 3.0f + phase) +
                       rng.range(-0.08f, 0.08f);
        if (i == static_cast<int>(seed % static_cast<std::uint32_t>(segments)))
            irregular[i] *= 0.78f;
    }
    const glm::vec3 apex(rng.range(-0.035f, 0.035f) * height,
                         height * 0.96f,
                         rng.range(-0.035f, 0.035f) * height);
    for (int i = 0; i < segments; ++i)
    {
        const int j = (i + 1) % segments;
        const float a0 = 2.0f * Pi * static_cast<float>(i) / segments;
        const float a1 = 2.0f * Pi * static_cast<float>(j) / segments;
        const glm::vec3 r0(std::cos(a0), 0.0f, std::sin(a0));
        const glm::vec3 r1(std::cos(a1), 0.0f, std::sin(a1));
        const glm::vec3 p0 = r0 * radius * irregular[i] +
                             glm::vec3(0.0f, -height * 0.025f, 0.0f);
        const glm::vec3 p1 = r1 * radius * irregular[j] +
                             glm::vec3(0.0f, -height * 0.025f, 0.0f);
        const glm::vec3 m0 = r0 * radius * irregular[i] * 0.48f +
                             glm::vec3(0.0f, height * 0.58f, 0.0f);
        const glm::vec3 m1 = r1 * radius * irregular[j] * 0.48f +
                             glm::vec3(0.0f, height * 0.58f, 0.0f);
        quad(mesh, p0, p1, m1, m0, glm::mix(dark, base, 0.45f),
             0.91f, 0.12f, 1.0f, 0.24f);
        triangle(mesh, m0, m1, apex, tip, 0.87f, 0.18f, 1.0f, 0.28f);
    }
}

VegetationMeshData makeTree(float height, float crownRadius, int tiers,
                            int segments, std::uint32_t seed, bool trunk,
                            bool oneTier)
{
    VegetationMeshData mesh;
    if (oneTier)
    {
        appendGroundedIrregularCone(mesh, height, crownRadius, segments, seed);
        mesh.updateBounds();
        return mesh;
    }
    if (trunk)
        appendTaperedPrism(mesh, height, std::max(5, segments));
    Random rng{seed};
    const int actualTiers = oneTier ? 1 : tiers;
    for (int tier = 0; tier < actualTiers; ++tier)
    {
        const float t = actualTiers == 1 ? 0.43f : static_cast<float>(tier) /
            static_cast<float>(actualTiers - 1);
        const float y = glm::mix(height * 0.16f, height * 0.88f, t);
        const float envelope = oneTier ? 0.82f : std::pow(1.0f - t, 0.62f);
        const float radius = crownRadius * (0.22f + 0.78f * envelope) *
                             rng.range(0.92f, 1.08f);
        const float thickness = height * glm::mix(0.13f, 0.075f, t);
        appendFoliageTier(mesh, y, radius, thickness, segments,
                          rng.range(0.0f, 2.0f * Pi), seed + tier * 7919u, height);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet treeSet(float h, float r, int tiers, std::uint32_t seed)
{
    VegetationMeshLODSet result;
    result.lod0 = makeTree(h, r, tiers, 8, seed, true, false);
    result.lod1 = makeTree(h, r, std::max(4, tiers - 2), 6, seed, true, false);
    result.lod2 = makeTree(h, r, 1, 6, seed, false, true);
    result.shadow = makeTree(h, r, 1, 6, seed + 17u, false, true);
    return result;
}

constexpr std::array<glm::vec3, 12> IcosaVertices = {{
    {-0.525731f, 0.850651f, 0.0f}, {0.525731f, 0.850651f, 0.0f},
    {-0.525731f,-0.850651f, 0.0f}, {0.525731f,-0.850651f, 0.0f},
    {0.0f,-0.525731f, 0.850651f}, {0.0f, 0.525731f, 0.850651f},
    {0.0f,-0.525731f,-0.850651f}, {0.0f, 0.525731f,-0.850651f},
    {0.850651f,0.0f,-0.525731f}, {0.850651f,0.0f,0.525731f},
    {-0.850651f,0.0f,-0.525731f}, {-0.850651f,0.0f,0.525731f}
}};
constexpr std::array<std::array<int, 3>, 20> IcosaFaces = {{
    {{0,11,5}},{{0,5,1}},{{0,1,7}},{{0,7,10}},{{0,10,11}},
    {{1,5,9}},{{5,11,4}},{{11,10,2}},{{10,7,6}},{{7,1,8}},
    {{3,9,4}},{{3,4,2}},{{3,2,6}},{{3,6,8}},{{3,8,9}},
    {{4,9,5}},{{2,4,11}},{{6,2,10}},{{8,6,7}},{{9,8,1}}
}};

void appendLobe(VegetationMeshData& mesh, const glm::vec3& center,
                const glm::vec3& scale, float warpPhase, float wind)
{
    const glm::vec3 bottom(0.035f, 0.080f, 0.020f);
    const glm::vec3 top(0.100f, 0.190f, 0.045f);
    for (const auto& face : IcosaFaces)
    {
        glm::vec3 p[3];
        for (int k = 0; k < 3; ++k)
        {
            glm::vec3 u = IcosaVertices[face[k]];
            if (u.y < 0.0f) u.y *= 0.25f;
            const float warp = 1.0f + 0.10f * std::sin(
                u.x * 5.3f + u.y * 3.7f + u.z * 4.1f + warpPhase);
            p[k] = center + u * scale * warp;
        }
        const float y = glm::clamp(((p[0].y + p[1].y + p[2].y) / 3.0f -
                       center.y) / std::max(scale.y, 0.01f) * 0.5f + 0.5f,
                       0.0f, 1.0f);
        triangle(mesh, p[0], p[1], p[2], glm::mix(bottom, top, y),
                 0.90f, wind * y, 1.0f, 0.18f);
    }
}

VegetationMeshData makeShrubMesh(int lobes, bool swept, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    for (int i = 0; i < lobes; ++i)
    {
        const float angle = swept ? 0.0f : 2.0f * Pi * i / lobes;
        const float along = swept ? (static_cast<float>(i) - (lobes - 1) * 0.5f) * 0.34f : 0.32f;
        const glm::vec3 center(std::cos(angle) * along, 0.30f +
            (swept ? i * 0.035f : (i == 0 ? 0.18f : 0.0f)),
            std::sin(angle) * along);
        appendLobe(mesh, center, glm::vec3(rng.range(0.42f, 0.66f),
            swept ? rng.range(0.28f, 0.42f) : rng.range(0.38f, 0.58f),
            rng.range(0.38f, 0.62f)), rng.range(0.0f, 20.0f), 0.55f);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet shrubSet(bool swept, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeShrubMesh(swept ? 4 : 5, swept, seed);
    r.lod1 = makeShrubMesh(2, swept, seed);
    r.lod2 = makeShrubMesh(1, swept, seed);
    r.shadow = r.lod2;
    return r;
}

void appendGrassBlade(VegetationMeshData& mesh, const glm::vec3& base,
                      float angle, float height, float width, float bend,
                      const glm::vec3& bendDirection)
{
    const glm::vec3 right(std::cos(angle), 0.0f, std::sin(angle));
    const glm::vec3 forward = glm::normalize(glm::vec3(-right.z, 0.0f, right.x) +
                                              bendDirection * 0.45f);
    const glm::vec3 mid = base + glm::vec3(0.0f, height * 0.58f, 0.0f) +
                          forward * bend * 0.35f;
    const glm::vec3 tip = base + glm::vec3(0.0f, height, 0.0f) + forward * bend;
    const glm::vec3 normal = glm::normalize(forward * 0.35f + glm::vec3(0.0f, 0.65f, 0.0f));
    const glm::vec3 root(0.025f, 0.065f, 0.012f);
    const glm::vec3 middle(0.060f, 0.160f, 0.025f);
    const glm::vec3 top(0.115f, 0.260f, 0.045f);
    const std::uint16_t first = static_cast<std::uint16_t>(mesh.vertices.size());
    vertex(mesh, base - right * width, normal, root, 0.96f, 0.0f, 0.65f);
    vertex(mesh, base + right * width, normal, root, 0.96f, 0.0f, 0.65f);
    vertex(mesh, mid - right * width * 0.72f, normal, middle, 0.94f, 0.48f, 0.72f);
    vertex(mesh, mid + right * width * 0.72f, normal, middle, 0.94f, 0.48f, 0.72f);
    vertex(mesh, tip, normal, top, 0.92f, 1.0f, 0.80f);
    mesh.indices.insert(mesh.indices.end(), {first, static_cast<std::uint16_t>(first + 1u),
        static_cast<std::uint16_t>(first + 3u), first,
        static_cast<std::uint16_t>(first + 3u), static_cast<std::uint16_t>(first + 2u),
        static_cast<std::uint16_t>(first + 2u), static_cast<std::uint16_t>(first + 3u),
        static_cast<std::uint16_t>(first + 4u)});
}

VegetationMeshData makeGrass(int blades, int style, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    for (int i = 0; i < blades; ++i)
    {
        const float angle = 2.0f * Pi * (static_cast<float>(i) / blades) + rng.range(-0.22f, 0.22f);
        glm::vec3 commonBend(0.0f);
        if (style == 2) commonBend = glm::vec3(0.85f, 0.0f, 0.32f);
        // A tuft represents a small patch rather than one hair-thin plant.
        // The terrain spans 8.2 km, so sub-decimetre blades disappear even in
        // the near field. Keep them recognisably low-poly but readable.
        const float radial = style == 1 ? rng.range(0.42f, 1.10f) : rng.range(0.18f, 0.88f);
        appendGrassBlade(mesh, glm::vec3(std::cos(angle) * radial, -0.015f,
            std::sin(angle) * radial), angle, rng.range(1.18f, 2.24f) *
            (i == blades - 1 ? 0.78f : 1.0f), rng.range(0.055f, 0.115f),
            rng.range(style == 0 ? 0.18f : 0.26f, style == 2 ? 0.52f : 0.42f), commonBend);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet grassSet(int style, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeGrass(style == 0 ? 12 : 10, style, seed);
    r.lod1 = makeGrass(8, style, seed);
    r.lod2 = makeGrass(5, style, seed);
    r.shadow = r.lod2;
    return r;
}

void appendStem(VegetationMeshData& mesh, float height, const glm::vec3& lean,
                const glm::vec3& base)
{
    const glm::vec3 stem(0.040f, 0.150f, 0.025f);
    for (int i = 0; i < 3; ++i)
    {
        const float a0 = 2.0f * Pi * i / 3.0f;
        const float a1 = 2.0f * Pi * (i + 1) / 3.0f;
        const glm::vec3 p0 = base + glm::vec3(std::cos(a0) * 0.028f, -0.06f,
                                               std::sin(a0) * 0.028f);
        const glm::vec3 p1 = base + glm::vec3(std::cos(a1) * 0.028f, -0.06f,
                                               std::sin(a1) * 0.028f);
        const glm::vec3 top0 = base + glm::vec3(std::cos(a0) * 0.028f, height,
                                                std::sin(a0) * 0.028f) + lean;
        const glm::vec3 top1 = base + glm::vec3(std::cos(a1) * 0.028f, height,
                                                std::sin(a1) * 0.028f) + lean;
        quad(mesh, p0, p1, top1, top0,
             stem, 0.94f, 0.55f, 0.65f, 0.45f);
    }
}

void appendPetal(VegetationMeshData& mesh, const glm::vec3& center, float angle,
                 float length, float width, const glm::vec3& color, bool down)
{
    const glm::vec3 d(std::cos(angle), down ? -0.35f : 0.18f, std::sin(angle));
    const glm::vec3 s(-std::sin(angle), 0.0f, std::cos(angle));
    const glm::vec3 inner = center + d * length * 0.10f;
    const glm::vec3 outer = center + d * length;
    quad(mesh, inner - s * width * 0.25f, inner + s * width * 0.25f,
         outer + s * width, outer - s * width, color, 0.72f, 1.0f, 0.75f, 0.65f);
}

void appendOctahedron(VegetationMeshData& mesh, const glm::vec3& center,
                      float radius, const glm::vec3& color, float wind)
{
    const glm::vec3 top = center + glm::vec3(0.0f, radius, 0.0f);
    const glm::vec3 bottom = center - glm::vec3(0.0f, radius, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        const float a0 = 2.0f * Pi * i / 4.0f;
        const float a1 = 2.0f * Pi * (i + 1) / 4.0f;
        const glm::vec3 p0 = center + glm::vec3(std::cos(a0) * radius, 0.0f, std::sin(a0) * radius);
        const glm::vec3 p1 = center + glm::vec3(std::cos(a1) * radius, 0.0f, std::sin(a1) * radius);
        triangle(mesh, top, p0, p1, color, 0.74f, wind, 0.75f, 0.35f);
        triangle(mesh, bottom, p1, p0, color, 0.74f, wind, 0.75f, 0.35f);
    }
}

VegetationMeshData makeFlower(int style, bool reduced, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    const float nominalHeight = style == 2 ? 2.35f : 1.90f;
    const int plantCount = reduced ? 2 : 4;
    for (int plant = 0; plant < plantCount; ++plant)
    {
        const float angle = rng.range(0.0f, 2.0f * Pi);
        const float radius = plant == 0 ? 0.0f : rng.range(0.42f, 1.18f);
        const glm::vec3 base(std::cos(angle) * radius, 0.0f,
                             std::sin(angle) * radius);
        const float h = nominalHeight * rng.range(0.84f, 1.18f);
        const glm::vec3 lean = style == 1
            ? glm::vec3(0.28f, -0.05f, 0.08f) * rng.range(0.78f, 1.22f)
            : glm::vec3(rng.range(-0.06f, 0.06f), 0.0f,
                        rng.range(-0.06f, 0.06f));
        appendStem(mesh, h, lean, base);

        if (style == 0)
        {
            const glm::vec3 color = rng.unit() > 0.45f
                ? glm::vec3(0.92f, 0.58f, 0.025f)
                : glm::vec3(0.96f, 0.86f, 0.72f);
            const int petals = reduced ? 3 : 5;
            const glm::vec3 center = base + glm::vec3(0.0f, h, 0.0f) + lean;
            for (int i = 0; i < petals; ++i)
                appendPetal(mesh, center, 2.0f * Pi * i / petals,
                            0.40f, 0.160f, color, false);
            appendOctahedron(mesh, center + glm::vec3(0.0f, 0.032f, 0.0f),
                             0.088f, glm::vec3(0.46f, 0.21f, 0.010f), 1.0f);
        }
        else if (style == 1)
        {
            const glm::vec3 center = base + glm::vec3(0.0f, h, 0.0f) + lean;
            const int petals = reduced ? 3 : 5;
            for (int i = 0; i < petals; ++i)
                appendPetal(mesh, center, 2.0f * Pi * i / petals,
                            0.38f, 0.150f,
                            glm::vec3(0.66f, 0.055f, 0.72f), true);
        }
        else
        {
            const int heads = reduced ? 2 : 3;
            for (int i = 0; i < heads; ++i)
                appendOctahedron(mesh, base + lean + glm::vec3(
                    (i & 1) ? 0.055f : -0.040f,
                    h - 0.13f + i * 0.19f, 0.0f),
                    0.118f - i * 0.010f,
                    glm::vec3(0.74f, 0.10f, 0.64f), 1.0f);
        }
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet flowerSet(int style, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeFlower(style, false, seed);
    r.lod1 = makeFlower(style, true, seed);
    r.lod2 = VegetationMeshData{};
    r.shadow = VegetationMeshData{};
    return r;
}

VegetationMeshData makeCushion(int lobes, bool elongated, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    for (int i = 0; i < lobes; ++i)
    {
        const float a = 2.0f * Pi * i / lobes + rng.range(-0.3f, 0.3f);
        appendLobe(mesh, glm::vec3(std::cos(a) * 0.18f, 0.08f,
            std::sin(a) * 0.18f), glm::vec3(elongated ? 0.38f : 0.31f,
            0.13f, elongated ? 0.23f : 0.31f), rng.range(0.0f, 30.0f), 0.08f);
    }
    const glm::vec3 edge(0.100f, 0.120f, 0.040f);
    const int leaves = std::max(6, lobes * 2);
    for (int i = 0; i < leaves; ++i)
    {
        const float a = 2.0f * Pi * i / leaves;
        const glm::vec3 d(std::cos(a), 0.0f, std::sin(a));
        const glm::vec3 s(-d.z, 0.0f, d.x);
        triangle(mesh, d * 0.18f - s * 0.035f, d * 0.50f,
                 d * 0.18f + s * 0.035f, edge, 0.94f, 0.04f, 0.55f, 0.65f);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet cushionSet(bool elongated, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeCushion(4, elongated, seed);
    r.lod1 = makeCushion(2, elongated, seed);
    r.lod2 = makeCushion(1, elongated, seed);
    r.shadow = r.lod2;
    return r;
}
}

void VegetationMeshData::updateBounds()
{
    if (vertices.empty()) { boundsMin = boundsMax = glm::vec3(0.0f); return; }
    boundsMin = glm::vec3(std::numeric_limits<float>::max());
    boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const auto& v : vertices)
    {
        boundsMin = glm::min(boundsMin, v.position);
        boundsMax = glm::max(boundsMax, v.position);
    }
}

VegetationMeshLODSet AlpineVegetationMeshFactory::makeTallConifer(std::uint32_t s) { return treeSet(45.0f, 11.5f, 7, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeBroadConifer(std::uint32_t s) { return treeSet(34.0f, 13.0f, 6, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeSapling(std::uint32_t s) { return treeSet(18.0f, 5.2f, 4, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeRoundShrub(std::uint32_t s) { return shrubSet(false, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeWindSweptShrub(std::uint32_t s) { return shrubSet(true, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeGrassTuftA(std::uint32_t s) { return grassSet(0, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeGrassTuftB(std::uint32_t s) { return grassSet(1, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeGrassTuftC(std::uint32_t s) { return grassSet(2, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeStarFlower(std::uint32_t s) { return flowerSet(0, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeBellFlower(std::uint32_t s) { return flowerSet(1, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeSpikeFlower(std::uint32_t s) { return flowerSet(2, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeCushionPlantA(std::uint32_t s) { return cushionSet(false, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeCushionPlantB(std::uint32_t s) { return cushionSet(true, s); }
