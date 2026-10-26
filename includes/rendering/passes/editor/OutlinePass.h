#pragma once

#include "core/Shader.h"
#include "scene/Camera.h"
#include "scene/SceneObject.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/resources/framebuffer/Gbuffer.h"

class OutlinePass {
public:
    OutlinePass(Framebuffer& hdrfbo, GBuffer& gbuffer, Shader& maskShader,
                Shader& outlineShader, Camera& camera,
                Screenquad& screenquad, int width, int height);

    OutlinePass(const OutlinePass&) = delete;
    OutlinePass& operator=(const OutlinePass&) = delete;

    ~OutlinePass();

    void resize(int width, int height);
    void render(const SceneObject* object, int width, int height);
    void renderMask(Shader& maskShader, const SceneObject* object,
                    int width, int height);
    void renderOutline( Shader& outlineShader, Screenquad& screenquad,
                    int width, int height);
    void drawObject(Shader& shader, const SceneObject* object);

    private:
    Shader& maskShader;
    Shader& outlineShader;
    Camera& camera;
    Framebuffer& hdrfbo;
    GBuffer& gbuffer;
    Screenquad& screenquad;

    GLuint maskFBO = 0;
    GLuint maskRBO = 0;
    // 存储黑白mask纹理id
    GLuint maskTexture = 0;

    int maskWidth = 0;
    int maskHeight = 0;

};
