#pragma once 

#include <vector>
#include <string>
#include <cstddef>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "core/Shader.h"

struct vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    glm::vec2 texCoords1;

    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct texture
{
    unsigned int id;
    std::string type;
    std::string path;
    unsigned int uvIndex = 0;
};

struct MaterialFactors
{
    glm::vec4 baseColor = glm::vec4(1.0f);
    float metallic = 0.0f;
    float roughness = 1.0f;
    bool alphaMask = false;
    float alphaCutoff = 0.5f;
};

class Mesh
{
public:
    std::vector<vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<texture> textures;
    MaterialFactors materialFactors;

    Mesh(
        std::vector<vertex> vertices,
        std::vector<unsigned int> indices,
        std::vector<texture> textures,
        MaterialFactors materialFactors = MaterialFactors()
    )
        : vertices(std::move(vertices)),
          indices(std::move(indices)),
          textures(std::move(textures)),
          materialFactors(materialFactors)
    {
        setupMesh();
    }

    void draw(Shader& shader);

    ~Mesh()
    {
        if (VAO != 0)
            glDeleteVertexArrays(1, &VAO);
        if (VBO != 0)
            glDeleteBuffers(1, &VBO);
        if (EBO != 0)
            glDeleteBuffers(1, &EBO);
    }

    Mesh(Mesh&& other) noexcept
    : vertices(std::move(other.vertices)),
      indices(std::move(other.indices)),
      textures(std::move(other.textures)),
      materialFactors(other.materialFactors),
      VAO(other.VAO), VBO(other.VBO), EBO(other.EBO)
    {
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
    }

    // 禁止拷贝
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void setupMesh();
};
