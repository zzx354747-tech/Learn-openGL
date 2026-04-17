#include "Graphics.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

CubeAttribute SetcubeAttribute;

void setCubeAttribute(float x, float y, float z)
{
    SetcubeAttribute.x = x;
    SetcubeAttribute.y = y;
    SetcubeAttribute.z = z;
};

void Cube(VertexData *vertices)
{
    float hx = SetcubeAttribute.x / 2.0f;
    float hy = SetcubeAttribute.y / 2.0f;
    float hz = SetcubeAttribute.z / 2.0f;

    glm::vec3 p000(-hx, -hy, -hz);
    glm::vec3 p001(-hx, -hy,  hz);
    glm::vec3 p010(-hx,  hy, -hz);
    glm::vec3 p011(-hx,  hy,  hz);
    glm::vec3 p100( hx, -hy, -hz);
    glm::vec3 p101( hx, -hy,  hz);
    glm::vec3 p110( hx,  hy, -hz);
    glm::vec3 p111( hx,  hy,  hz);

    VertexData temp [36] =
    {
        { p000, { 0.0f,  0.0f, -1.0f }, {0.0f, 0.0f} },
        { p100, { 0.0f,  0.0f, -1.0f }, {1.0f, 0.0f} },
        { p110, { 0.0f,  0.0f, -1.0f }, {1.0f, 1.0f} },
        { p110, { 0.0f,  0.0f, -1.0f }, {1.0f, 1.0f} },
        { p010, { 0.0f,  0.0f, -1.0f }, {0.0f, 1.0f} },
        { p000, { 0.0f,  0.0f, -1.0f }, {0.0f, 0.0f} },

        // front
        { p001, { 0.0f,  0.0f,  1.0f }, {0.0f, 0.0f} },
        { p101, { 0.0f,  0.0f,  1.0f }, {1.0f, 0.0f} },
        { p111, { 0.0f,  0.0f,  1.0f }, {1.0f, 1.0f} },
        { p111, { 0.0f,  0.0f,  1.0f }, {1.0f, 1.0f} },
        { p011, { 0.0f,  0.0f,  1.0f }, {0.0f, 1.0f} },
        { p001, { 0.0f,  0.0f,  1.0f }, {0.0f, 0.0f} },

        // left
        { p011, {-1.0f,  0.0f,  0.0f }, {1.0f, 0.0f} },
        { p010, {-1.0f,  0.0f,  0.0f }, {1.0f, 1.0f} },
        { p000, {-1.0f,  0.0f,  0.0f }, {0.0f, 1.0f} },
        { p000, {-1.0f,  0.0f,  0.0f }, {0.0f, 1.0f} },
        { p001, {-1.0f,  0.0f,  0.0f }, {0.0f, 0.0f} },
        { p011, {-1.0f,  0.0f,  0.0f }, {1.0f, 0.0f} },

        // right
        { p111, { 1.0f,  0.0f,  0.0f }, {1.0f, 0.0f} },
        { p110, { 1.0f,  0.0f,  0.0f }, {1.0f, 1.0f} },
        { p100, { 1.0f,  0.0f,  0.0f }, {0.0f, 1.0f} },
        { p100, { 1.0f,  0.0f,  0.0f }, {0.0f, 1.0f} },
        { p101, { 1.0f,  0.0f,  0.0f }, {0.0f, 0.0f} },
        { p111, { 1.0f,  0.0f,  0.0f }, {1.0f, 0.0f} },

        // bottom
        { p000, { 0.0f, -1.0f,  0.0f }, {0.0f, 1.0f} },
        { p100, { 0.0f, -1.0f,  0.0f }, {1.0f, 1.0f} },
        { p101, { 0.0f, -1.0f,  0.0f }, {1.0f, 0.0f} },
        { p101, { 0.0f, -1.0f,  0.0f }, {1.0f, 0.0f} },
        { p001, { 0.0f, -1.0f,  0.0f }, {0.0f, 0.0f} },
        { p000, { 0.0f, -1.0f,  0.0f }, {0.0f, 1.0f} },

        // top
        { p010, { 0.0f,  1.0f,  0.0f }, {0.0f, 1.0f} },
        { p110, { 0.0f,  1.0f,  0.0f }, {1.0f, 1.0f} },
        { p111, { 0.0f,  1.0f,  0.0f }, {1.0f, 0.0f} },
        { p111, { 0.0f,  1.0f,  0.0f }, {1.0f, 0.0f} },
        { p011, { 0.0f,  1.0f,  0.0f }, {0.0f, 0.0f} },
        { p010, { 0.0f,  1.0f,  0.0f }, {0.0f, 1.0f} }
    };

    for (int i = 0; i < 36; i++)
    {
        vertices[i] = temp[i];
    }
};

glm::vec3 CubePositions[10];

void setCubePosition(glm::vec3 *cubePositions)
{
    cubePositions[0] = glm::vec3( 0.0f,  0.0f,  0.0f); 
    cubePositions[1] = glm::vec3( 2.0f,  5.0f, -15.0f); 
    cubePositions[2] = glm::vec3(-1.5f, -2.2f, -2.5f);  
    cubePositions[3] = glm::vec3(-3.8f, -2.0f, -12.3f);  
    cubePositions[4] = glm::vec3( 2.4f, -0.4f, -3.5f);  
    cubePositions[5] = glm::vec3(-1.7f,  3.0f, -7.5f);  
    cubePositions[6] = glm::vec3( 1.3f, -2.0f, -2.5f);  
    cubePositions[7] = glm::vec3( 1.5f,  2.0f, -2.5f); 
    cubePositions[8] = glm::vec3( 1.5f,  0.2f, -1.5f); 
    cubePositions[9] = glm::vec3(-1.3f,  1.0f, -1.5f);
};


