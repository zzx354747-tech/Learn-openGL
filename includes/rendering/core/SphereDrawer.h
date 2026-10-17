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
    void drawInstance(
    Shader& shader,
    const glm::mat4& matrix,
    unsigned int materialIndex);
    
    // 第一个 const：限制传入的 glm::mat4 矩阵不能通过这个参数被修改。
    // 第二个 const：限制这个成员函数不能修改 SphereDrawer 对象的成员变量。
    void drawPick(Shader& shader, const glm::mat4&) const;

private:
    void drawOne(Shader& shader, unsigned int index);
    bool isDefaultScene() const;

    SphereMesh*        sphereMesh;
    SceneRenderState*  state;
    SceneRenderConfig* config;

    Material materials[MaterialSphereCount];
};
