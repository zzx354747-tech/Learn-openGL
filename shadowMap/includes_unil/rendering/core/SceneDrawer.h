#pragma once

#include "glad/gl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/shader.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/core/SceneRenderTypes.h"

class SceneDrawer
{
public:
    SceneDrawer (CubeMesh* cubeMesh, PlaneMesh* planeMesh, SceneRenderState* state)
        :cubeMesh(cubeMesh), planeMesh(planeMesh), state(state)
    {
    }

    void drawScene(Shader& shader)
    {
        drawCubes(shader);
        drawPlane(shader);
    }

    void drawCubes(Shader& shader)
    {
        if (!cubeMesh || !state)
            return;

        for (unsigned int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, state->cubePositions[i]);

            float angle = 20.0f * i;
            model = glm::rotate(
                model,
                glm::radians(angle),
                glm::vec3(1.0f, 0.3f, 0.5f)
            );

            shader.setMat4("model", model);
            cubeMesh->draw();
        }
    }

    void drawPlane(Shader& shader)
    {
        if (!planeMesh)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        planeMesh->draw();
    }


private:
    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    SceneRenderState* state = nullptr;

};
