#include "scene/SceneObjectManager.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

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

SceneObject* SceneObjectManager::getSceneObject(SceneObjectID id)
{
    for (auto& obj: m_sceneObjects)
    {
        if (obj.id == id)
        {
            return &obj;
        }
    }
    return nullptr;
}

SceneObjectID SceneObjectManager::importModel(const std::string& path, SceneSelection scene,
                                             const glm::vec3& position, float fitSize)
{
    const std::filesystem::path file = std::filesystem::weakly_canonical(std::filesystem::u8path(path));
    if (!std::filesystem::is_regular_file(file))
        throw std::runtime_error("Model file does not exist.");
    std::string extension = file.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".glb" && extension != ".gltf" && extension != ".obj" &&
        extension != ".fbx" && extension != ".dae" && extension != ".stl" && extension != ".ply")
        throw std::runtime_error("Supported models: GLB, glTF, OBJ, FBX, DAE, STL, PLY.");
    if (m_nextID == InvalidSceneObjectID)
        throw std::runtime_error("Scene object IDs exhausted.");

    const std::string key = file.generic_string();
    auto found = m_importedModels.find(key);
    if (found == m_importedModels.end())
    {
        auto model = std::make_unique<ModelDrawer>(key);
        if (!model->isLoaded())
            throw std::runtime_error(model->getLoadError().empty() ? "Model contains no drawable meshes." : model->getLoadError());
        model->centerPivot();
        found = m_importedModels.emplace(key, std::move(model)).first;
    }
    ModelDrawer* drawer = found->second.get();
    const glm::vec3 extent = drawer->getBoundsMax() - drawer->getBoundsMin();
    const float longest = std::max({extent.x, extent.y, extent.z});
    if (!std::isfinite(longest) || longest < 1e-6f)
        throw std::runtime_error("Model has invalid or empty bounds.");
    SceneObject object;
    object.id = m_nextID;
    object.name = file.stem().string() + " (" + std::to_string(object.id) + ")";
    object.ref = drawer;
    object.scene = scene;
    object.transform.position = position;
    object.transform.scale = glm::vec3(fitSize / longest);
    m_sceneObjects.push_back(object);
    ++m_nextID;
    return object.id;
}

void SceneObjectManager::drawObjects(Shader& shader, SceneSelection scene, bool spheres,
                                    bool models, bool sphereMaterials) const
{
    for (const auto& object : m_sceneObjects)
    {
        if (!object.isVisible || object.scene != scene) continue;
        const glm::mat4 matrix = object.transform.matrix();
        if (const auto* sphere = std::get_if<sphereRenderRef>(&object.ref))
        {
            if (!spheres || !sphere->drawer) continue;
            if (sphereMaterials) sphere->drawer->drawInstance(shader, matrix, sphere->materialIndex);
            else sphere->drawer->drawPick(shader, matrix);
        }
        else if (const auto* model = std::get_if<ModelDrawer*>(&object.ref))
        {
            if (models && *model) (*model)->draw(shader, matrix);
        }
    }
}
