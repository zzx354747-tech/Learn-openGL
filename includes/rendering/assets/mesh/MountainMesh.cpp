#include <cstddef>
#include "rendering/assets/mesh/MountainMesh.h"

MountainMesh::MountainMesh(int resolution, float spacing)
    : resolution(resolution),
      spacing(spacing)
{
}

void MountainMesh::generate()
{
    vertices.clear();
    indices.clear();

    float textureWorldSize = 0.5f;

    for (int z = 0; z < resolution; ++z)
    {
        for (int x = 0; x < resolution; ++x)
        {
            MountainVertex vertex{};
            float worldY = getHeight(x * spacing, z * spacing);
            vertex.position = glm::vec3(x * spacing, worldY, z * spacing);
            vertex.normal = glm::vec3(0.0f, 0.0f, 0.0f);
            vertex.texCoords = glm::vec2(vertex.position.x / textureWorldSize,
                                        vertex.position.z / textureWorldSize);
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

float MountainMesh::getHeight(float x, float z) const
{
    return 15.0f
        * std::sin(0.2f * x)
        * std::cos(0.2f * z);
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
