#pragma once
#include "core/Shader.h"
#include "rendering/assets/mesh/SphereMesh.h"
#include "rendering/assets/texture/Material.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/resources/render/SceneRenderTypes.h"

class SphereDrawer
{
public:
    SphereDrawer(SphereMesh*        sphereMesh,
                 SceneRenderState*  state,
                 SceneRenderConfig* config);

    void loadMaterials(const std::string& baseDir);
    void draw(Shader& shader);
    void drawBasic(Shader& shader);
    void drawReflect(Shader& shader);

private:
    void drawOne(Shader& shader, unsigned int index);
    bool isDefaultScene() const;

    SphereMesh*        sphereMesh;
    SceneRenderState*  state;
    SceneRenderConfig* config;

    Material materials[MaterialSphereCount];
};
