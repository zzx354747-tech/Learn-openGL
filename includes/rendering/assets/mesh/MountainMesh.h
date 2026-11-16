#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

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
    glm::vec3 getBoundsMin() const { return boundsMin; }
    glm::vec3 getBoundsMax() const { return boundsMax; }

private:
    void setupMesh();
    void initResources();
    void generateBranchSamplePoints();
    float getHeight(float x, float z) const;
    float perlinNoise(float x, float z) const;
    float fade(float t) const;
    float getMountainMask(float x, float z) const;
    float getMacroHeight(float x, float z) const;
    float getBaseHeight(float x, float z) const;
    float getRidgeHeight(float x, float z) const;
    float getSpineCenter(float x) const;
    float getSpineAmplitude(float x) const;
    float getSpineHeight(float x, float z) const;
    float getBranchHeight(float x, float z) const;
    std::vector<glm::vec2> sampleCatmullRom(const std::array<glm::vec2, 4>& controlPoints) const;
    std::array<std::vector<glm::vec2>, 4> branchSamplePoints;
    glm::vec2 getGradient(int x, int z) const;

    int resolution;
    float spacing;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};

    std::vector<MountainVertex> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
};
