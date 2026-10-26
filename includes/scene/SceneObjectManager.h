#pragma once

#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/core/SphereDrawer.h"
#include "scene/SceneObject.h"
#include <vector>
#include <memory>
#include <unordered_map>

class SceneObjectManager
{
public:
    SceneObjectManager(ModelDrawer* livingRoomDrawer, SphereDrawer* sphereDrawer)
        : m_livingRoomDrawer(livingRoomDrawer), m_sphereDrawer(sphereDrawer) {}
    ~SceneObjectManager() = default;

    void addSceneObject();
    std::vector<SceneObjectData> describeObjects() const;
    // Build on a temporary manager, then commit with swapContents after all resources load.
    void restoreObjects(const std::vector<SceneObjectData>& objects);
    void swapContents(SceneObjectManager& other) noexcept;

    SceneObject* getSceneObject(SceneObjectID id);

    SceneObjectID importModel(const std::string& path, SceneSelection scene,
                              const glm::vec3& position, float fitSize = 2.0f);
    void drawObjects(Shader& shader, SceneSelection scene, bool spheres = true,
                     bool models = true, bool sphereMaterials = false) const;

    // 第一个const,既不复制容器，也不能通过这个引用修改容器和容器里的元素
    // 第二个const,函数既不能修改类的成员变量，也不能调用非const成员函数
    const std::vector<SceneObject>& getSceneObjects() const 
    { return m_sceneObjects; }
        
private:
    std::vector<SceneObject> m_sceneObjects;
    ModelDrawer* m_livingRoomDrawer;
    SphereDrawer* m_sphereDrawer;
    std::unordered_map<std::string, std::unique_ptr<ModelDrawer>> m_importedModels;
    SceneObjectID m_nextID = MaterialSphereCount + 2;
};
   
