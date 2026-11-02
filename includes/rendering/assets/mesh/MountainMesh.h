#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

struct MountainVertex
{
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    glm::vec2 texCoords{0.0f};
    glm::vec3 tangent{0.0f};
    glm::vec3 bitangent{0.0f};
};

class MountainMesh
{
public:
    MountainMesh(int resolution, float spacing);
    ~MountainMesh();

    MountainMesh(const MountainMesh&) = delete;
    MountainMesh& operator=(const MountainMesh&) = delete;

    void generate();
    void draw() const;

private:
    void setupMesh();
    void initResources();
    float getHeight(float x, float z) const;

    int resolution;
    float spacing;

    std::vector<MountainVertex> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
};
