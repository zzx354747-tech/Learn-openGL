#include "rendering/core/SphereDrawer.h"
#include <glm/gtc/matrix_transform.hpp>

SphereDrawer::SphereDrawer(SphereMesh*        sphereMesh,
                           SceneRenderState*  state,
                           SceneRenderConfig* config)
    : sphereMesh(sphereMesh), state(state), config(config)
{
}

void SphereDrawer::loadMaterials(const std::string& baseDir)
{
    static const char* dirs[MaterialSphereCount] = {
        "Bricks066_2K-PNG",
        "Grass005_2K-PNG",
        "Gravel023_2K-PNG",
        "Marble012_2K-PNG",
        "Metal003_2K-PNG",
        "Metal034_2K-PNG",
        "Metal055A_2K-PNG",
        "Rock060_2K-PNG",
    };

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
        materials[i] = Material::loadFromDirectory(baseDir + dirs[i]);
}

void SphereDrawer::draw(Shader& shader)
{
    if (!isDefaultScene() || !sphereMesh || !state)
        return;

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
        drawOne(shader, i);
}

void SphereDrawer::drawOne(Shader& shader, unsigned int index)
{
    if (index >= MaterialSphereCount)
        return;

    materials[index].bind(shader);

    glm::mat4 model(1.0f);
    model = glm::translate(model, state->materialSpherePositions[index]);
    model = glm::scale(model, glm::vec3(0.65f));
    shader.setMat4("model", model);

    sphereMesh->draw();
}

void SphereDrawer::drawBasic(Shader& shader)
{
    if (!isDefaultScene() || !sphereMesh || !state)
        return;

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
    {
        materials[i].bindAlbedoOnly(shader);   // 只绑 albedo，unlit forward 只需要这个

        glm::mat4 model(1.0f);
        model = glm::translate(model, state->materialSpherePositions[i]);
        model = glm::scale(model, glm::vec3(0.65f));
        shader.setMat4("model", model);

        sphereMesh->draw();
    }
}

void SphereDrawer::drawReflect(Shader& shader)
{
    if (!isDefaultScene() || !sphereMesh || !state)
        return;

    for (unsigned int i = 0; i < MaterialSphereCount; ++i)
    {
        // 纯反射：不需要材质贴图，只需要 model matrix 算法线
        glm::mat4 model(1.0f);
        model = glm::translate(model, state->materialSpherePositions[i]);
        model = glm::scale(model, glm::vec3(0.65f));
        shader.setMat4("model", model);

        sphereMesh->draw();
    }
}

bool SphereDrawer::isDefaultScene() const
{
    return !config || config->sceneSelection == SceneSelection::Default;
}
