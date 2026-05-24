#ifndef GRAPHICS_H
#define GRAPHICS_H
#include <glm/glm.hpp>
struct VertexData
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct CubeAttribute
{
    float x;
    float y;
    float z;
};

void setCubeAttribute(float x, float y, float z);
void Cube(VertexData *vertices);
void setCubePosition(glm::vec3 *cubePositions);

#endif
