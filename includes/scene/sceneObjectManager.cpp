#include "scene/sceneObjectManager.h"

void SceneObjectManager::addSceneObject()
{
    // 名称顺序对应 SphereDrawer::loadMaterials() 中的材质编号。
    const char* sphereNames[MaterialSphereCount] = {
        "Sphere_bricks066",
        "Sphere_grass005",
        "Sphere_gravel023",
        "Sphere_marble012",
        "Sphere_metal003",
        "Sphere_metal034",
        "Sphere_metal055A",
        "Sphere_rock060",
    };

    // 使用当前 SceneRenderState 中的小球初始位置。
    const SceneRenderState initialState;

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
    {
        SceneObject sphere{};
        sphere.id = i + 1;
        sphere.name = sphereNames[i];
        sphere.ref = sphereRenderRef{m_sphereDrawer, i};
        sphere.transform.position = initialState.materialSpherePositions[i];
        sphere.transform.rotationDegrees = glm::vec3(0.0f);
        sphere.transform.scale = glm::vec3(0.65f);
        sphere.scene = SceneSelection::Default;
        sphere.isVisible = true;
        m_sceneObjects.push_back(sphere);
    }

    // 与 RendererScene 中实际加载的客厅模型及其初始变换保持一致。
    SceneObject model_livingroom{};
    model_livingroom.id = MaterialSphereCount + 1;
    model_livingroom.name = "model_livingroom";
    model_livingroom.ref = m_livingRoomDrawer;
    model_livingroom.transform.position = glm::vec3(0.0f, 0.0f, -3.0f);
    model_livingroom.transform.rotationDegrees = glm::vec3(0.0f);
    model_livingroom.transform.scale = glm::vec3(0.2f);
    model_livingroom.scene = SceneSelection::LivingRoom;
    model_livingroom.isVisible = true;
    m_sceneObjects.push_back(model_livingroom);
}
