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

    glm::vec3 tangent;
    glm::vec3 bitangent;
};

struct texture
{
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh
{
public:
    std::vector<vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<texture> textures;

    Mesh(
        std::vector<vertex> vertices,
        std::vector<unsigned int> indices,
        std::vector<texture> textures
    )
        : vertices(vertices),
          indices(indices),
          textures(textures)
    {
        setupMesh();
    }

    void draw(Shader& shader);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void setupMesh();
};