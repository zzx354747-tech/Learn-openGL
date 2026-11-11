#include "rendering/core/MountainDrawer.h"

MountainDrawer::MountainDrawer(
    MountainMesh* mountainMesh,
    SceneRenderState* state,
    SceneRenderConfig* config
)
    : mountainMesh(mountainMesh), state(state), config(config)
{
    material = Material::loadFromDirectory(
    "../textures/PBR/Grass005_2K-PNG"
);
}

void MountainDrawer::draw(Shader& shader, const glm::mat4& transform) const
{
    if (!isMountainScene())
        return;
    if (!mountainMesh)
        return;

    // use grass material for mountain
    material.bind(shader);
    shader.setBool("alphaMask", false);
    shader.setMat4("model", transform);
    mountainMesh->draw();
}

void MountainDrawer::drawBasic(Shader& shader, const glm::mat4& transform) const
{
    if (!isMountainScene())
        return;
    if (!mountainMesh)
        return;
    material.bindAlbedoOnly(shader);
    shader.setBool("alphaMask", false);
    shader.setMat4("model", transform);
    mountainMesh->draw();
}

void MountainDrawer::drawReflect(Shader& shader, const glm::mat4& transform) const
{
    drawDepth(shader, transform);
}

void MountainDrawer::drawDepth(Shader& shader, const glm::mat4& transform) const
{
    if (!isMountainScene())
        return;
    if (!mountainMesh)
        return;
    shader.setMat4("model", transform);
    mountainMesh->draw();
}

bool MountainDrawer::isMountainScene() const
{
    return config && config->sceneSelection == SceneSelection::Mountain;
}
