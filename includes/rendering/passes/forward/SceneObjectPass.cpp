#include "rendering/passes/forward/SceneObjectPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"

void SceneObjectPass::renderSceneObjectPass(
    Camera&               camera,
    SceneRenderResources& resources,
    SceneRenderConfig&    config,
    SphereDrawer&         sphereDrawer,
    int                   bfwidth,
    int                   bfheight)
{
    // Light 模式不归这个 pass 管——那是延迟管线（GeometryPass + LightingPass）的活
    if (config.forwardLightMode == ForwardLightMode::Light)
        return;

    Shader* shader = config.forwardLightMode == ForwardLightMode::Basic
        ? resources.basicForwardShader
        : resources.reflectForwardShader;

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

    config.forwardLightMode == ForwardLightMode::Basic
        ? sphereDrawer.drawBasic(*shader)
        : sphereDrawer.drawReflect(*shader);
}
