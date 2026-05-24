#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Shader.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderTypes.h"

class SceneDrawer
{
public:
    SceneDrawer (CubeMesh* cubeMesh, 
        PlaneMesh* planeMesh, 
        SceneRenderState* state,
        Model* model)
        :cubeMesh(cubeMesh), planeMesh(planeMesh), state(state), model(model)
    {
    }

    void drawScene(Shader& shader)
    {
        drawCubes(shader);
        drawPlane(shader);
        drawModel(shader);
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

    void drawModel(Shader& shader)
    {
        if (!model)
            return;

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        if (model->hasValidBounds())
        {
            glm::vec3 boundsCenter = model->getBoundsCenter();
            glm::vec3 boundsSize = model->getBoundsSize();
            float maxExtent = glm::max(boundsSize.x, glm::max(boundsSize.y, boundsSize.z));
            float scale = maxExtent > 0.0f ? 10.0f / maxExtent : 1.0f;
            float floorY = -0.5f;
            glm::vec3 targetCenter(
                0.0f,
                floorY + boundsSize.y * scale * 0.5f,
                -2.0f
            );

            modelMatrix = glm::translate(modelMatrix, targetCenter);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
            modelMatrix = glm::translate(modelMatrix, -boundsCenter);
        }
        shader.setMat4("model", modelMatrix);

        model->draw(shader);
    }


private:
    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    SceneRenderState* state = nullptr;
    Model* model = nullptr;

};
