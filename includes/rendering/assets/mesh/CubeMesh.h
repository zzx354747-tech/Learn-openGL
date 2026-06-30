#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

class CubeMesh
{
public:
    // 定义一个完整的顶点结构体
    struct Vertex {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    CubeMesh();

    void draw() const;

    ~CubeMesh();

    CubeMesh(const CubeMesh&) = delete;
    CubeMesh& operator=(const CubeMesh&) = delete;

private:
    unsigned int cubeVAO = 0, cubeVBO = 0;
};

