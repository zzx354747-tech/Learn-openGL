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
        SceneRenderConfig* config,
        Model* model,
        Model* sponzaModel,
        Model* sibenikModel)
        : cubeMesh(cubeMesh),
          planeMesh(planeMesh),
          state(state),
          config(config),
          model(model),
          sponzaModel(sponzaModel),
          sibenikModel(sibenikModel)
    {
    }

    void drawScene(Shader& shader)
    {
        drawCubes(shader);
        drawSecondCubes(shader);
        drawPlane(shader);
        drawModel(shader);
    }

    void drawCubes(Shader& shader)
    {
        if (!isDefaultScene())
            return;

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

    void drawSecondCubes(Shader& shader)
    {
        if (!isDefaultScene())
            return;

        if (!cubeMesh || !state)
            return;

        for (unsigned int i = 0; i < 3; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, state->secondCubePositions[i] + glm::vec3(0.0f, 1.0f, 0.0f));

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
        if (!isDefaultScene())
            return;

        if (!planeMesh)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        planeMesh->draw();
    }

    void drawModel(Shader& shader)
    {
        Model* activeModel = getActiveModel();
        if (!activeModel)
            return;

        shader.setMat4("model", getActiveModelMatrix());

        activeModel->draw(shader);
    }

    glm::mat4 getActiveModelMatrix() const
    {
        Model* activeModel = getActiveModel();
        if (!activeModel || !activeModel->hasValidBounds())
            return glm::mat4(1.0f);

        glm::vec3 boundsCenter = activeModel->getBoundsCenter();
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float scale = getActiveModelScale(activeModel);
        glm::vec3 targetCenter = getActiveModelTargetCenter(activeModel, scale);

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, targetCenter);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
        modelMatrix = glm::translate(modelMatrix, -boundsCenter);

        return modelMatrix;
    }

    glm::vec3 getActiveSceneWorldCenter() const
    {
        Model* activeModel = getActiveModel();
        if (!activeModel || !activeModel->hasValidBounds())
            return glm::vec3(0.0f);

        float scale = getActiveModelScale(activeModel);
        return getActiveModelTargetCenter(activeModel, scale);
    }

    glm::vec3 getActiveSceneWorldSize() const
    {
        Model* activeModel = getActiveModel();
        if (!activeModel || !activeModel->hasValidBounds())
            return glm::vec3(1.0f);

        return activeModel->getBoundsSize() * getActiveModelScale(activeModel);
    }


private:
    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    SceneRenderState* state = nullptr;
    SceneRenderConfig* config = nullptr;
    Model* model = nullptr;
    Model* sponzaModel = nullptr;
    Model* sibenikModel = nullptr;

    bool isDefaultScene() const
    {
        return !config || config->sceneSelection == SceneSelection::Default;
    }

    float getActiveModelScale(Model* activeModel) const
    {
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float maxExtent = glm::max(boundsSize.x, glm::max(boundsSize.y, boundsSize.z));
        float targetSize = isDefaultScene() ? 10.0f : 18.0f;

        return maxExtent > 0.0f ? targetSize / maxExtent : 1.0f;
    }

    glm::vec3 getActiveModelTargetCenter(Model* activeModel, float scale) const
    {
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float floorY = -0.5f;

        return glm::vec3(
            0.0f,
            floorY + boundsSize.y * scale * 0.5f,
            isDefaultScene() ? -2.0f : -1.0f
        );
    }

    Model* getActiveModel() const
    {
        if (!config)
            return model;

        switch (config->sceneSelection)
        {
            case SceneSelection::Sponza:
                return sponzaModel;
            case SceneSelection::Sibenik:
                return sibenikModel;
            case SceneSelection::Default:
            default:
                return model;
        }

    }

};
