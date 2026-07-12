#include "rendering/passes/forward/SceneObjectPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"

void SceneObjectPass::renderSceneObjectPass(
    Camera&               camera,
    SceneRenderResources& resources,
    SceneRenderConfig&    config,
    SphereDrawer&         sphereDrawer,
    ModelDrawer&          modelDrawer,
    int                   bfwidth,
    int                   bfheight)
{
    // Light 模式不归这个 pass 管——那是延迟管线（GeometryPass + LightingPass）的活
    if (config.forwardLightMode == ForwardLightMode::Light)
        return;

    Shader* shader = resources.shaderLibrary
        ? (config.forwardLightMode == ForwardLightMode::Basic
            ? &resources.shaderLibrary->basicForward
            : &resources.shaderLibrary->cubemap)
        : nullptr;

    if (!shader)
        return;

    shader->use();

    CameraUniformSetter::apply(*shader, camera, bfwidth, bfheight);  // Owner A，reflect 需要 cameraPos 算反射向量
    if (config.forwardLightMode == ForwardLightMode::Reflect)
    {
        if (!resources.skybox)
            return;

        shader->setBool("isSkybox", false);
        shader->setInt("skybox", 0);
        resources.skybox->bind(0);
    }

    if (config.forwardLightMode == ForwardLightMode::Basic)
    {
        sphereDrawer.drawBasic(*shader);
    }
    else
    {
        sphereDrawer.drawReflect(*shader);
    }

    modelDrawer.draw(*shader);
}
