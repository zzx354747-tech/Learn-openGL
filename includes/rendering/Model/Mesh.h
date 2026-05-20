#pragma once 

#include <vector>
#include <string>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include "core/shader.h"

struct vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
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

    Mesh(std::vector<vertex> vertices, std::vector<unsigned int> indices, std::vector<texture> textures)
        : vertices(vertices), indices(indices), textures(textures)
    {
        setupMesh();
    }

    void draw(Shader& shader);

private:
    unsigned int VAO, VBO, EBO;

    void setupMesh();
};
