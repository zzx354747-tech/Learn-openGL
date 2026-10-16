#pragma once

#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/core/SphereDrawer.h"
#include "scene/sceneObject.h"
#include <vector>

class SceneObjectManager
{
public:
    SceneObjectManager(ModelDrawer* livingRoomDrawer, SphereDrawer* sphereDrawer)
        : m_livingRoomDrawer(livingRoomDrawer), m_sphereDrawer(sphereDrawer) {}
    ~SceneObjectManager() = default;

    void addSceneObject();

    SceneObject* getSceneObject(SceneObjectID id);
        
private:
    std::vector<SceneObject> m_sceneObjects;
    ModelDrawer* m_livingRoomDrawer;
    SphereDrawer* m_sphereDrawer;
};
   
