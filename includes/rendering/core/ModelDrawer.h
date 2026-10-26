#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "rendering/modelload/Model.h"
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
    bool isLoaded() const { return model.isLoaded(); }
    const std::string& getLoadError() const { return model.getLoadError(); }
    glm::vec3 getBoundsMin() const { return model.getBoundsMin(); }
    glm::vec3 getBoundsMax() const { return model.getBoundsMax(); }
    void centerPivot();

    void draw(Shader& shader, const glm::mat4& transform) const;
    void draw(Shader& shader) const;

private:
    Model model;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 assetTransform = glm::mat4(1.0f);
    SceneRenderConfig* config = nullptr;
    SceneSelection visibleScene = SceneSelection::LivingRoom;
};
