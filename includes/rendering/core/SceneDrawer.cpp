#include "rendering/core/SceneDrawer.h"

SceneDrawer::SceneDrawer (CubeMesh* cubeMesh, PlaneMesh* planeMesh, SphereMesh* sphereMesh, SceneRenderState* state, SceneRenderConfig* config, Model* model, Model* modernCityModel) : cubeMesh(cubeMesh), planeMesh(planeMesh), sphereMesh(sphereMesh), state(state), config(config), model(model), modernCityModel(modernCityModel)
{
    }

void SceneDrawer::drawScene(Shader& shader)
{
        drawPlane(shader);
        drawMaterialSpheres(shader);
        drawModel(shader);
    }

void SceneDrawer::drawCubes(Shader& shader)
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

void SceneDrawer::drawSecondCubes(Shader& shader)
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

void SceneDrawer::drawPlane(Shader& shader)
{
        if (!isDefaultScene())
            return;

        if (!planeMesh)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        shader.setMat4("model", model);
        planeMesh->draw();
    }

void SceneDrawer::drawMaterialSpheres(Shader& shader)
{
        if (!isDefaultScene() || !sphereMesh || !state)
            return;

        for (unsigned int i = 0; i < MaterialSphereCount; ++i)
        {
            drawMaterialSphere(shader, i);
        }
    }

void SceneDrawer::drawClearSphere(Shader& shader)
{
        if (!isDefaultScene() || !sphereMesh || !state || !config || !config->enableClearSphere)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, state->clearSpherePosition);
        model = glm::scale(model, glm::vec3(0.34f));
        shader.setMat4("model", model);
        sphereMesh->draw();
    }

void SceneDrawer::drawMaterialSphere(Shader& shader, unsigned int index)
{
        if (!isDefaultScene() || !sphereMesh || !state || index >= MaterialSphereCount)
            return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, state->materialSpherePositions[index]);
        model = glm::scale(model, glm::vec3(0.65f));
        shader.setMat4("model", model);
        sphereMesh->draw();
    }

void SceneDrawer::drawModel(Shader& shader)
{
        if (config && !config->enableModel)
            return;

        Model* activeModel = getActiveModel();
        if (!activeModel)
            return;

        shader.setMat4("model", getActiveModelMatrix());

        activeModel->draw(shader);
    }

glm::mat4 SceneDrawer::getActiveModelMatrix() const
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

glm::vec3 SceneDrawer::getActiveSceneWorldCenter() const
{
        Model* activeModel = getActiveModel();
        glm::vec3 sphereCenter = getMaterialSphereSceneCenter();

        if ((config && !config->enableModel) || !activeModel || !activeModel->hasValidBounds())
            return sphereCenter;

        float scale = getActiveModelScale(activeModel);
        glm::vec3 modelCenter = getActiveModelTargetCenter(activeModel, scale);
        glm::vec3 modelHalfSize = activeModel->getBoundsSize() * scale * 0.5f;
        glm::vec3 sphereHalfSize = getMaterialSphereSceneSize() * 0.5f;

        glm::vec3 minBounds = glm::min(modelCenter - modelHalfSize, sphereCenter - sphereHalfSize);
        glm::vec3 maxBounds = glm::max(modelCenter + modelHalfSize, sphereCenter + sphereHalfSize);

        return (minBounds + maxBounds) * 0.5f;
    }

glm::vec3 SceneDrawer::getActiveSceneWorldSize() const
{
        Model* activeModel = getActiveModel();
        glm::vec3 sphereSize = getMaterialSphereSceneSize();
        if ((config && !config->enableModel) || !activeModel || !activeModel->hasValidBounds())
            return sphereSize;

        float scale = getActiveModelScale(activeModel);
        glm::vec3 modelCenter = getActiveModelTargetCenter(activeModel, scale);
        glm::vec3 modelSize = activeModel->getBoundsSize() * getActiveModelScale(activeModel);
        glm::vec3 sphereCenter = getMaterialSphereSceneCenter();
        glm::vec3 modelHalfSize = modelSize * 0.5f;
        glm::vec3 sphereHalfSize = sphereSize * 0.5f;

        glm::vec3 minBounds = glm::min(modelCenter - modelHalfSize, sphereCenter - sphereHalfSize);
        glm::vec3 maxBounds = glm::max(modelCenter + modelHalfSize, sphereCenter + sphereHalfSize);

        return maxBounds - minBounds;
    }

bool SceneDrawer::isDefaultScene() const
{
        return !config || config->sceneSelection == SceneSelection::Default;
    }

float SceneDrawer::getActiveModelScale(Model* activeModel) const
{
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float maxExtent = glm::max(boundsSize.x, glm::max(boundsSize.y, boundsSize.z));
        float targetSize = isDefaultScene() ? 15.0f : 18.0f;

        return maxExtent > 0.0f ? targetSize / maxExtent : 1.0f;
    }

glm::vec3 SceneDrawer::getActiveModelTargetCenter(Model* activeModel, float scale) const
{
        glm::vec3 boundsSize = activeModel->getBoundsSize();
        float floorY = -0.5f;

        return glm::vec3(
            0.0f,
            floorY + boundsSize.y * scale * 0.5f,
            isDefaultScene() ? -5.5f : -1.0f
        );
    }

glm::vec3 SceneDrawer::getMaterialSphereSceneSize() const
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

        glm::vec3 size(maxX - minX + 2.0f, 3.0f, maxZ - minZ + 2.0f);
        if (config && config->enableClearSphere)
        {
            size.x = glm::max(size.x, glm::abs(state->clearSpherePosition.x - minX) + 2.0f);
            size.z = glm::max(size.z, glm::abs(state->clearSpherePosition.z - minZ) + 2.0f);
        }

        return size;
    }

glm::vec3 SceneDrawer::getMaterialSphereSceneCenter() const
{
        if (!state)
            return glm::vec3(0.0f);

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

        if (config && config->enableClearSphere)
        {
            minX = glm::min(minX, state->clearSpherePosition.x);
            maxX = glm::max(maxX, state->clearSpherePosition.x);
            minZ = glm::min(minZ, state->clearSpherePosition.z);
            maxZ = glm::max(maxZ, state->clearSpherePosition.z);
        }

        return glm::vec3(
            (minX + maxX) * 0.5f,
            0.6f,
            (minZ + maxZ) * 0.5f
        );
    }

Model* SceneDrawer::getActiveModel() const
{
        if (!config)
            return model;

        switch (config->sceneSelection)
        {
            case SceneSelection::ModernCity:
                return modernCityModel;
            case SceneSelection::Default:
            default:
                return model;
        }

    }
