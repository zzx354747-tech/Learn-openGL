#include <cstddef>
#include <cmath>
#include "rendering/assets/mesh/MountainMesh.h"

#include <array>
#include <limits>

static const std::array<std::array<glm::vec2, 4>, 4> branchControls = {{
    // 支脉 1
    {{
        {1000.0f, 2000.0f},
        { 940.0f, 1880.0f},
        { 820.0f, 1750.0f},
        { 750.0f, 1620.0f}
    }},

    // 支脉 2
    {{
        {1500.0f, 2000.0f},
        {1580.0f, 2130.0f},
        {1740.0f, 2300.0f},
        {1800.0f, 2450.0f}
    }},

    // 支脉 3
    {{
        {2200.0f, 2000.0f},
        {2280.0f, 1850.0f},
        {2420.0f, 1710.0f},
        {2500.0f, 1540.0f}
    }},

    // 支脉 4
    {{
        {2800.0f, 2000.0f},
        {2880.0f, 2110.0f},
        {2990.0f, 2260.0f},
        {3100.0f, 2400.0f}
    }}
}};

MountainMesh::MountainMesh(int resolution, float spacing)
    : resolution(resolution),
      spacing(spacing)
{
}

void MountainMesh::generate()
{
    vertices.clear();
    indices.clear();
    boundsMin = glm::vec3(0.0f);
    boundsMax = glm::vec3(0.0f);

    generateBranchSamplePoints();

    float textureWorldSize = 20.0f;

    for (int z = 0; z < resolution; ++z)
    {
        for (int x = 0; x < resolution; ++x)
        {
            MountainVertex vertex{};
            float worldY = getHeight(x * spacing, z * spacing);
            vertex.position = glm::vec3(x * spacing, worldY, z * spacing);
            if (vertices.empty()) boundsMin = boundsMax = vertex.position;
            else
            {
                boundsMin = glm::min(boundsMin, vertex.position);
                boundsMax = glm::max(boundsMax, vertex.position);
            }
            vertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
            vertex.texCoords = glm::vec2(
            vertex.position.x / textureWorldSize,
            vertex.position.z / textureWorldSize
            );
            vertices.push_back(vertex);
        }
    }

    for (int z = 0; z < resolution - 1; ++z)
    {
        for (int x = 0; x < resolution - 1; ++x)
        {
            unsigned int topLeft = z * resolution + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * resolution + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    for (std::size_t i = 0; i < indices.size(); i += 3)
    {
        auto& a = vertices[indices[i]];
        auto& b = vertices[indices[i + 1]];
        auto& c = vertices[indices[i + 2]];

        glm::vec3 edge1 = b.position - a.position;
        glm::vec3 edge2 = c.position - a.position;

        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        a.normal += faceNormal;
        b.normal += faceNormal;
        c.normal += faceNormal;
    }

    for (auto& vertex : vertices)
    {
        if (glm::dot(vertex.normal, vertex.normal) > 1e-12f)
            vertex.normal = glm::normalize(vertex.normal);
        else
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Derive the tangent basis from the existing positions, UVs and triangles.
    // Do not change the grid size, UV orientation or topology.
    for (std::size_t i = 0; i < indices.size(); i += 3)
    {
        auto& a = vertices[indices[i]];
        auto& b = vertices[indices[i + 1]];
        auto& c = vertices[indices[i + 2]];
        const glm::vec3 edge1 = b.position - a.position;
        const glm::vec3 edge2 = c.position - a.position;
        const glm::vec2 uv1 = b.texCoords - a.texCoords;
        const glm::vec2 uv2 = c.texCoords - a.texCoords;
        const float determinant = uv1.x * uv2.y - uv1.y * uv2.x;
        if (glm::abs(determinant) < 1e-12f) continue;
        const glm::vec3 tangent = (edge1 * uv2.y - edge2 * uv1.y) / determinant;
        const glm::vec3 bitangent = (edge2 * uv1.x - edge1 * uv2.x) / determinant;
        a.tangent += tangent; b.tangent += tangent; c.tangent += tangent;
        a.bitangent += bitangent; b.bitangent += bitangent; c.bitangent += bitangent;
    }
    for (auto& vertex : vertices)
    {
        const glm::vec3 tangent = vertex.tangent - vertex.normal * glm::dot(vertex.normal, vertex.tangent);
        vertex.tangent = glm::dot(tangent, tangent) > 1e-12f
            ? glm::normalize(tangent) : glm::vec3(1.0f, 0.0f, 0.0f);
        const glm::vec3 perpendicular = glm::cross(vertex.normal, vertex.tangent);
        const float handedness = glm::dot(perpendicular, vertex.bitangent) < 0.0f ? -1.0f : 1.0f;
        vertex.bitangent = handedness * glm::normalize(perpendicular);
    }

    setupMesh();
}

void MountainMesh::setupMesh()
{
    if (VAO == 0 && VBO == 0 && EBO == 0)
    {
        initResources();
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
        vertices.size() * sizeof(MountainVertex),
        vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

void MountainMesh::generateBranchSamplePoints()
{
    for (std::size_t i = 0; i < branchControls.size(); ++i)
    {
        branchSamplePoints[i] = sampleCatmullRom(branchControls[i]);
    }
}

float MountainMesh::getHeight(float x, float z) const
{
    float mask = getMountainMask(x, z);

    float spine = getSpineHeight(x, z);
    float base = getBaseHeight(x, z);
    float ridge = getRidgeHeight(x, z);
    float branch = getBranchHeight(x, z);

    constexpr float baseWeight = 0.15f;
    constexpr float ridgeWeight = 0.10f;

    float rawHeight =
    branch +
    baseWeight * base +
    ridgeWeight * ridge;

    return mask * rawHeight;
}

float MountainMesh::getMacroHeight(float x, float z) const
{
    return getBaseHeight(x, z) + getRidgeHeight(x, z);
}

float MountainMesh::getBaseHeight(float x, float z) const
{
    constexpr float frequency = 0.0005f;
    constexpr float amplitude = 400.0f;
    const float noise = glm::clamp(perlinNoise(x * frequency, z * frequency) / 0.5353f, -1.0f, 1.0f);
    // Ordinary low-frequency Perlin mapped to [0, 1]: broad rises and lowlands.
    return amplitude * (0.5f + 0.5f * noise);
}

float MountainMesh::getRidgeHeight(float x, float z) const
{
    constexpr float frequency = 0.001f;
    constexpr float amplitude = 200.0f;
    // Offset the ridge field so it does not share the base terrain's lattice.
    const float noise = glm::clamp(
        perlinNoise(x * frequency + 17.3f, z * frequency + 43.7f) / 0.5353f, -1.0f, 1.0f);
    const float ridge = 1.0f - std::abs(noise);
    return amplitude * ridge;
}

void MountainMesh::initResources()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glVertexAttribPointer(0, 3,
        GL_FLOAT, GL_FALSE,
        sizeof(MountainVertex), (
        void*)offsetof(MountainVertex, position));

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3,
        GL_FLOAT, GL_FALSE,
        sizeof(MountainVertex),
        (void*)offsetof(MountainVertex, normal));

    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2,
        GL_FLOAT, GL_FALSE,
        sizeof(MountainVertex),
        (void*)offsetof(MountainVertex, texCoords));

    glEnableVertexAttribArray(2);

    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(MountainVertex),
                          (void*)offsetof(MountainVertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(MountainVertex),
                          (void*)offsetof(MountainVertex, bitangent));
    glEnableVertexAttribArray(5);
}

void MountainMesh::draw() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(indices.size()),
        GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

MountainMesh::~MountainMesh()
{
    if (VAO != 0)
        glDeleteVertexArrays(1, &VAO);
    if (VBO != 0)
        glDeleteBuffers(1, &VBO);
    if (EBO != 0)
        glDeleteBuffers(1, &EBO);
}

float MountainMesh::perlinNoise(float x, float z) const
{
    int x0 = static_cast<int>(std::floor(x));
    int z0 = static_cast<int>(std::floor(z));

    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float tx = x - static_cast<float>(x0);
    float tz = z - static_cast<float>(z0);

    glm::vec2 g00 = getGradient(x0, z0);
    glm::vec2 g10 = getGradient(x1, z0);
    glm::vec2 g01 = getGradient(x0, z1);
    glm::vec2 g11 = getGradient(x1, z1);
    glm::vec2 d00 = glm::vec2(tx, tz);
    glm::vec2 d10 = glm::vec2(tx - 1.0f, tz);
    glm::vec2 d01 = glm::vec2(tx, tz - 1.0f);
    glm::vec2 d11 = glm::vec2(tx - 1.0f, tz - 1.0f);

    float n00 = glm::dot(g00, d00);
    float n10 = glm::dot(g10, d10);
    float n01 = glm::dot(g01, d01);
    float n11 = glm::dot(g11, d11);

    float u = fade(tx);
    float v = fade(tz);

    float nx0 = glm::mix(n00, n10, u);
    float nx1 = glm::mix(n01, n11, u);
    float nxy = glm::mix(nx0, nx1, v);
    return nxy;
}

float MountainMesh::getMountainMask(float x, float z) const
{
    float warpFrequency = 0.001f;
    float warpAmplitude = 250.0f;

    float offsetX = warpAmplitude *
        perlinNoise(x * warpFrequency, z * warpFrequency);

    float offsetZ = warpAmplitude *
        perlinNoise(
            x * warpFrequency + 37.2f,
            z * warpFrequency + 91.7f
        );

    float warpedX = x + offsetX;
    float warpedZ = z + offsetZ;

    float dx = warpedX - 2000.0f;
    float dz = warpedZ - 2000.0f;

    float a = 1700.0f;
    float b = 650.0f;

    float distance = std::sqrt(dx * dx/ (a * a) + dz * dz / (b * b));

    float mountainMask =
    1.0f - glm::smoothstep(0.0f, 1.0f, distance);

    return mountainMask;
}

float MountainMesh::getSpineCenter(float x) const
{
    constexpr float centerZ = 2000.0f;
    constexpr float bendAmplitude = 350.0f;
    constexpr float bendFrequency = 0.0007f;

    float n = perlinNoise(
        x * bendFrequency,
        12.3f
    );

    return centerZ + bendAmplitude * n;
}

float MountainMesh::getSpineAmplitude(float x) const
{
    constexpr float baseAmplitude = 550.0f;
    constexpr float variationAmplitude = 180.0f;
    constexpr float variationFrequency = 0.001f;

    float n = perlinNoise(
        x * variationFrequency,
        23.7f
    );

    return baseAmplitude + variationAmplitude * n;
}

float MountainMesh::getSpineHeight(float x, float z) const
{
    constexpr float halfWidth = 800.0f;

    float centerZ = getSpineCenter(x);
    float distance = std::abs(z - centerZ);

    float falloff =
        1.0f - glm::smoothstep(0.0f, halfWidth, distance);

    float amplitude = getSpineAmplitude(x);

    return amplitude * falloff;
}

float MountainMesh::fade(float t) const
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float MountainMesh::getBranchHeight(float x, float z) const
{
    glm::vec2 point(x, z);

    constexpr float amplitudes[4] = {
        180.0f, 250.0f, 220.0f, 160.0f
    };

    constexpr float halfWidths[4] = {
        120.0f, 160.0f, 140.0f, 110.0f
    };

    float maxHeight = 0.0f;

    for (std::size_t branch = 0; branch < branchSamplePoints.size(); ++branch)
    {
        const auto& samplePoints = branchSamplePoints[branch];

        float minDistance = std::numeric_limits<float>::max();

        for (std::size_t i = 0; i + 1 < samplePoints.size(); ++i)
        {
            glm::vec2 A = samplePoints[i];
            glm::vec2 B = samplePoints[i + 1];

            glm::vec2 AB = B - A;
            glm::vec2 AP = point - A;

            float lengthSquared = glm::dot(AB, AB);

            if (lengthSquared < 1e-12f)
                continue;

            float t = glm::clamp(
                glm::dot(AP, AB) / lengthSquared,
                0.0f,
                1.0f
            );

            glm::vec2 closestPoint = A + t * AB;

            float distance = glm::length(point - closestPoint);

            minDistance = glm::min(minDistance, distance);
        }

        float falloff = 1.0f - glm::smoothstep(
            0.0f,
            halfWidths[branch],
            minDistance
        );

        float height = amplitudes[branch] * falloff;

        maxHeight = glm::max(maxHeight, height);
    }

    return maxHeight;
}

std::vector<glm::vec2> MountainMesh::sampleCatmullRom(const std::array<glm::vec2, 4>& controlPoints) const
{
    std::vector<glm::vec2> points;

    constexpr int samplesPerSegment = 8;
    points.reserve(3 * samplesPerSegment + 1);

    for (int i = 0; i < 3; ++i)
    {
        glm::vec2 P1 = controlPoints[i];
        glm::vec2 P2 = controlPoints[i + 1];

        glm::vec2 P0 = (i == 0)
            ? 2.0f * P1 - P2
            : controlPoints[i - 1];

        glm::vec2 P3 = (i == 2)
            ? 2.0f * P2 - P1
            : controlPoints[i + 2];

        glm::vec2 M1 = 0.5f * (P2 - P0);
        glm::vec2 M2 = 0.5f * (P3 - P1);

        glm::vec2 a = 2.0f * P1 - 2.0f * P2 + M1 + M2;
        glm::vec2 b = -3.0f * P1 + 3.0f * P2 - 2.0f * M1 - M2;
        glm::vec2 c = M1;
        glm::vec2 d = P1;

        for (int j = 0; j < samplesPerSegment; ++j)
        {
            float t = static_cast<float>(j) / samplesPerSegment;

            float t2 = t * t;
            float t3 = t2 * t;

            glm::vec2 point = a * t3 + b * t2 + c * t + d;

            points.push_back(point);
        }
    }

    points.push_back(controlPoints[3]);

    return points;
}



glm::vec2 MountainMesh::getGradient(int x, int z) const
{
    unsigned int hash =
        static_cast<unsigned int>(x) * 374761393u +
        static_cast<unsigned int>(z) * 668265263u;

    hash ^= hash >> 13;
    hash *= 1274126177u;
    hash ^= hash >> 16;

    switch (hash % 4u)
    {
        case 0: return glm::vec2( 1.0f,  0.0f);
        case 1: return glm::vec2(-1.0f,  0.0f);
        case 2: return glm::vec2( 0.0f,  1.0f);
        default: return glm::vec2(0.0f, -1.0f);
    }
}
