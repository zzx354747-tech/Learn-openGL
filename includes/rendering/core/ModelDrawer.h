#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "rendering/modelload/Model.h"
#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "core/Shader.h"

class ModelDrawer
{
public:
    explicit ModelDrawer(const std::string& path);
    ModelDrawer(const std::string& path, SceneRenderConfig* config);

    void setTransform(const glm::mat4& transform);
    void setSceneConfig(SceneRenderConfig* config);
    void setVisibleInScene(SceneSelection scene);
    void setTerrainMesh(TerrainMesh* terrain);

    void draw(Shader& shader) const;

private:
    Model model;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    SceneRenderConfig* config = nullptr;
    SceneSelection visibleScene = SceneSelection::LivingRoom;
    TerrainMesh* terrainMesh = nullptr;
};
