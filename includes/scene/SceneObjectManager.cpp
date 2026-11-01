#include "scene/SceneObjectManager.h"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>

void SceneObjectManager::addSceneObject()
{
    // 名称顺序对应 SphereDrawer::loadMaterials() 中的材质编号。
    const char* sphereNames[MaterialUseCount] = {
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

    for (unsigned int i = 0; i < MaterialUseCount; ++i)
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
    model_livingroom.id = MaterialUseCount + 1;
    model_livingroom.name = "model_livingroom";
    model_livingroom.ref = m_livingRoomDrawer;
    model_livingroom.transform.position = glm::vec3(0.0f, 0.0f, -3.0f);
    model_livingroom.transform.rotationDegrees = glm::vec3(0.0f);
    model_livingroom.transform.scale = glm::vec3(0.2f);
    model_livingroom.scene = SceneSelection::LivingRoom;
    model_livingroom.isVisible = true;
    m_sceneObjects.push_back(model_livingroom);

    SceneObject model_mountain{};
    model_mountain.id = MaterialUseCount + 2;
    model_mountain.name = "model_mountain";
    model_mountain.ref = m_mountainDrawer;
    model_mountain.transform.position = glm::vec3(0.0f, -1.0f, 0.0f);
    model_mountain.transform.rotationDegrees = glm::vec3(0.0f);
    model_mountain.transform.scale = glm::vec3(1.0f);
    model_mountain.scene = SceneSelection::Mountain;
    model_mountain.isVisible = true;
    m_sceneObjects.push_back(model_mountain);
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
                                    bool models, bool mountains, SphereDrawMode sphereMode) const
{
    for (const auto& object : m_sceneObjects)
    {
        if (!object.isVisible || object.scene != scene) continue;
        const glm::mat4 matrix = object.transform.matrix();
        if (const auto* sphere = std::get_if<sphereRenderRef>(&object.ref))
        {
            if (!spheres || !sphere->drawer) continue;
            switch (sphereMode)
            {
            case SphereDrawMode::Basic:
                sphere->drawer->drawBasic(shader, matrix, sphere->materialIndex);
                break;
            case SphereDrawMode::Reflect:
                sphere->drawer->drawReflect(shader, matrix);
                break;
            case SphereDrawMode::Material:
                sphere->drawer->drawInstance(shader, matrix, sphere->materialIndex);
                break;
            case SphereDrawMode::Depth:
                sphere->drawer->drawPick(shader, matrix);
                break;
            }
        }
        else if (const auto* model = std::get_if<ModelDrawer*>(&object.ref))
        {
            if (models && *model) (*model)->draw(shader, matrix);
        }
        else if (const auto* mountain = std::get_if<MountainDrawer*>(&object.ref))
        {
            if (!mountains || !*mountain) continue;
            switch (sphereMode)
            {
            case SphereDrawMode::Material:
                (*mountain)->draw(shader, matrix);
                break;
            case SphereDrawMode::Basic:
                (*mountain)->drawBasic(shader, matrix);
                break;
            case SphereDrawMode::Reflect:
                (*mountain)->drawReflect(shader, matrix);
                break;
            case SphereDrawMode::Depth:
                (*mountain)->drawDepth(shader, matrix);
                break;
            }
        }
    }
}

std::vector<SceneObjectData> SceneObjectManager::describeObjects() const
{
    std::vector<SceneObjectData> result;
    for (const auto& o : m_sceneObjects)
    {
        SceneObjectData data;
        data.id = o.id; data.name = o.name; data.transform = o.transform;
        data.scene = o.scene; data.isVisible = o.isVisible;
        if (const auto* sphere = std::get_if<sphereRenderRef>(&o.ref))
        {
            data.type = "sphere";
            data.materialIndex = sphere->materialIndex;
        }
        else if (const auto* model = std::get_if<ModelDrawer*>(&o.ref))
        {
            const auto drawer = *model;
            if (drawer == m_livingRoomDrawer) data.type = "livingRoom";
            else
            {
                data.type = "model";
                for (const auto& resource : m_importedModels)
                    if (resource.second.get() == drawer) { data.modelPath = resource.first; break; }
                if (data.modelPath.empty()) throw std::runtime_error("Model has no persistent source.");
            }
        }
        else if (const auto* mountain = std::get_if<MountainDrawer*>(&o.ref))
        {
            data.type = "mountain";
        }
        result.push_back(std::move(data));
    }
    return result;
}

void SceneObjectManager::restoreObjects(const std::vector<SceneObjectData>& objects)
{
    SceneObjectManager next(m_livingRoomDrawer, m_sphereDrawer, m_mountainDrawer);
    SceneObjectID nextID = MaterialUseCount + 3;
    for (const auto& data : objects)
    {
        SceneObject object;
        if (data.type == "sphere") object.ref = sphereRenderRef{m_sphereDrawer, data.materialIndex};
        else if (data.type == "livingRoom")
        {
            if (m_livingRoomDrawer && !m_livingRoomDrawer->isLoaded())
                throw std::runtime_error("Built-in living room model is unavailable.");
            object.ref = m_livingRoomDrawer;
        }
        else if (data.type == "model")
        {
            next.importModel(data.modelPath, data.scene, glm::vec3(0));
            object.ref = next.m_sceneObjects.back().ref;
            next.m_sceneObjects.pop_back();
        }
        else if (data.type == "mountain")
        {
            if (!m_mountainDrawer)
                throw std::runtime_error("Built-in mountain drawer is unavailable.");
            object.ref = m_mountainDrawer;
        }
        else throw std::runtime_error("Unknown object type.");
        object.id = data.id; object.name = data.name; object.transform = data.transform;
        object.scene = data.scene; object.isVisible = data.isVisible;
        next.m_sceneObjects.push_back(std::move(object));
        nextID = std::max(nextID, data.id + 1);
    }
    next.m_nextID = nextID;
    swapContents(next);
}

void SceneObjectManager::swapContents(SceneObjectManager& other) noexcept
{
    m_sceneObjects.swap(other.m_sceneObjects);
    m_importedModels.swap(other.m_importedModels);
    std::swap(m_nextID, other.m_nextID);
}
