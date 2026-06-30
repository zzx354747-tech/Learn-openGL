#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

#include <glad/gl.h>
#include <glm/glm.hpp>

class SphereMesh
{
public:
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    SphereMesh(unsigned int xSegments = 64, unsigned int ySegments = 32);

    void draw() const;

    ~SphereMesh();

    SphereMesh(const SphereMesh&) = delete;
    SphereMesh& operator=(const SphereMesh&) = delete;

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;
    unsigned int indexCount = 0;
};
