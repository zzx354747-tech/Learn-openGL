#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/resources/render/SceneRenderTypes.h"
#include "core/Shader.h"
#include "rendering/assets/texture/Material.h"
#include "rendering/assets/mesh/MountainMesh.h"

class MountainDrawer
{
public:
    MountainDrawer(
        MountainMesh* mountainMesh,
        SceneRenderState* state,
        SceneRenderConfig* config
    );

    void draw(Shader& shader, const glm::mat4& transform) const;
    void drawBasic(Shader& shader, const glm::mat4& transform) const;
    void drawReflect(Shader& shader, const glm::mat4& transform) const;
    void drawDepth(Shader& shader, const glm::mat4& transform) const;
    bool isMountainScene() const;

private:
    MountainMesh* mountainMesh;
    SceneRenderState* state;
    SceneRenderConfig* config;
    Material material;
};
