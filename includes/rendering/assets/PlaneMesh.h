#pragma once
#include <glad/gl.h>

class PlaneMesh
{
public:
    PlaneMesh()
    {
        float planeVertices[] = 
    {
        // positions          // texCoords
    5.0f, -0.5f,  5.0f,   2.0f, 0.0f,
    -5.0f, -0.5f,  5.0f,   0.0f, 0.0f,
    -5.0f, -0.5f, -5.0f,   0.0f, 2.0f,

    5.0f, -0.5f,  5.0f,   2.0f, 0.0f,
    -5.0f, -0.5f, -5.0f,   0.0f, 2.0f,
    5.0f, -0.5f, -5.0f,   2.0f, 2.0f
    };

        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void draw() const
    {
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    ~PlaneMesh()
    {
        glDeleteBuffers(1, &planeVBO);
        glDeleteVertexArrays(1, &planeVAO);
    }

    PlaneMesh(const PlaneMesh&) = delete;
    PlaneMesh& operator=(const PlaneMesh&) = delete;

    private:
        unsigned int planeVAO = 0, planeVBO = 0;
};