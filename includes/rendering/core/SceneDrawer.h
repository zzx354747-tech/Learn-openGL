#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Shader.h"
#include "rendering/assets/CubeMesh.h"
#include "rendering/assets/PlaneMesh.h"
#include "rendering/assets/SphereMesh.h"
#include "rendering/Model/Model.h"
#include "rendering/core/SceneRenderTypes.h"

class SceneDrawer
{
public:
    SceneDrawer (CubeMesh* cubeMesh,
        PlaneMesh* planeMesh,
        SphereMesh* sphereMesh,
        SceneRenderState* state,
        SceneRenderConfig* config,
        Model* model)
        : cubeMesh(cubeMesh),
          planeMesh(planeMesh),
          sphereMesh(sphereMesh),
          state(state),
          config(config),
          model(model)
    {
    }

    void drawScene(Shader& shader)
    {
        drawPlane(shader);
        drawMaterialSpheres(shader);
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

    void drawMaterialSpheres(Shader& shader)
    {
        if (!isDefaultScene() || !sphereMesh || !state)
            return;

        for (unsigned int i = 0; i < MaterialSphereCount; ++i)
        {
            drawMaterialSphere(shader, i);
        }
    }

    void drawMaterialSphere(Shader& shader, unsigned int index)
    {
        if (!isDefaultScene() || !sphereMesh || !state || index >= MaterialSphereCount)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, state->materialSpherePositions[index]);
        model = glm::scale(model, glm::vec3(0.5f));
        shader.setMat4("model", model);
        sphereMesh->draw();
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
            return getMaterialSphereSceneSize();

        glm::vec3 modelSize = activeModel->getBoundsSize() * getActiveModelScale(activeModel);
        glm::vec3 sphereSize = getMaterialSphereSceneSize();

        return glm::max(modelSize, sphereSize);
    }


private:
    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    SphereMesh* sphereMesh = nullptr;
    SceneRenderState* state = nullptr;
    SceneRenderConfig* config = nullptr;
    Model* model = nullptr;

    bool isDefaultScene() const
    {
        return !config || config->sceneSelection == SceneSelection::Default;
    }

    float getActiveModelScale(Model* activeModel) const
    {
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float maxExtent = glm::max(boundsSize.x, glm::max(boundsSize.y, boundsSize.z));
        float targetSize = isDefaultScene() ? 15.0f : 18.0f;

        return maxExtent > 0.0f ? targetSize / maxExtent : 1.0f;
    }

    glm::vec3 getActiveModelTargetCenter(Model* activeModel, float scale) const
    {
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float floorY = -0.5f;

        return glm::vec3(
            0.0f,
            floorY + boundsSize.y * scale * 0.5f,
            isDefaultScene() ? -5.5f : -1.0f
        );
    }

    glm::vec3 getMaterialSphereSceneSize() const
    {
        if (!state)
            return glm::vec3(1.0f);

        float minX = state->materialSpherePositions[0].x;
        float maxX = state->materialSpherePositions[0].x;
        float minZ = state->materialSpherePositions[0].z;
        float maxZ = state->materialSpherePositions[0].z;

        for (unsigned int i = 1; i < MaterialSphereCount; ++i)
        {
            minX = glm::min(minX, state->materialSpherePositions[i].x);
            maxX = glm::max(maxX, state->materialSpherePositions[i].x);
            minZ = glm::min(minZ, state->materialSpherePositions[i].z);
            maxZ = glm::max(maxZ, state->materialSpherePositions[i].z);
        }

        return glm::vec3(maxX - minX + 2.0f, 3.0f, maxZ - minZ + 2.0f);
    }

    Model* getActiveModel() const
    {
        if (!config)
            return model;

        switch (config->sceneSelection)
        {
            case SceneSelection::Default:
            default:
                return model;
        }

    }

};
