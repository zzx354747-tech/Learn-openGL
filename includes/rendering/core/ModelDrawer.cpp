#include "rendering/core/ModelDrawer.h"

ModelDrawer::ModelDrawer(const std::string& path)
    : model(path)
{
}

ModelDrawer::ModelDrawer(const std::string& path, SceneRenderConfig* config)
    : model(path)
    , config(config)
{
}

void ModelDrawer::setTransform(const glm::mat4& transform)
{
    modelMatrix = transform;
}

void ModelDrawer::setSceneConfig(SceneRenderConfig* nextConfig)
{
    config = nextConfig;
}

void ModelDrawer::setVisibleInScene(SceneSelection scene)
{
    visibleScene = scene;
}

void ModelDrawer::setTerrainMesh(TerrainMesh* terrain)
{
    terrainMesh = terrain;
}

void ModelDrawer::draw(Shader& shader) const
{
    if (config && config->sceneSelection == SceneSelection::FujiTerrain)
    {
        if (terrainMesh)
            terrainMesh->draw(shader);
        return;
    }

    if (config && config->sceneSelection != visibleScene)
        return;

    // model 矩阵：per-object owner，既不属于 CameraUniformSetter 也不属于 RenderParams
    model.Draw(shader, modelMatrix);
}
