#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/Shader.h"
#include "rendering/assets/mesh/CubeMesh.h"
#include "rendering/assets/mesh/PlaneMesh.h"
#include "rendering/assets/mesh/SphereMesh.h"
#include "rendering/modelload/Model.h"
#include "rendering/resources/render/SceneRenderTypes.h"

class SceneDrawer
{
public:
    SceneDrawer(
        CubeMesh* cubeMesh,
        PlaneMesh* planeMesh,
        SphereMesh* sphereMesh,
        SceneRenderState* state,
        SceneRenderConfig* config,
        Model* model,
        Model* modernCityModel);

    void drawScene(Shader& shader);

    void drawCubes(Shader& shader);

    void drawSecondCubes(Shader& shader);

    void drawPlane(Shader& shader);

    void drawMaterialSpheres(Shader& shader);

    void drawClearSphere(Shader& shader);

    void drawMaterialSphere(Shader& shader, unsigned int index);

    void drawModel(Shader& shader);

    glm::mat4 getActiveModelMatrix() const;

    glm::vec3 getActiveSceneWorldCenter() const;

    glm::vec3 getActiveSceneWorldSize() const;


private:
    CubeMesh* cubeMesh = nullptr;
    PlaneMesh* planeMesh = nullptr;
    SphereMesh* sphereMesh = nullptr;
    SceneRenderState* state = nullptr;
    SceneRenderConfig* config = nullptr;
    Model* model = nullptr;
    Model* modernCityModel = nullptr;

    bool isDefaultScene() const;

    float getActiveModelScale(Model* activeModel) const;

    glm::vec3 getActiveModelTargetCenter(Model* activeModel, float scale) const;

    glm::vec3 getMaterialSphereSceneSize() const;

    glm::vec3 getMaterialSphereSceneCenter() const;

    Model* getActiveModel() const;

};
