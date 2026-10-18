#pragma once

#include "core/Shader.h"
#include "scene/Camera.h"
#include "scene/sceneObject.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"

class OutlinePass {
public:
    OutlinePass(Shader& shader, Camera& camera);

    // Caller supplies an object from the current scene. Shader uniforms:
    // model, view, projection, outlineColor.
    void render(Framebuffer& hdrfbo, const SceneObject* object, int width, int height);

    private:
    Shader& shader;
    Camera& camera;
};
