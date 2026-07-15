#pragma once

#include <glm/glm.hpp>

#include "core/Shader.h"
#include "scene/Camera.h"
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/resources/framebuffer/HDR_Framebuffer.h"
#include "rendering/resources/framebuffer/TemporalAAFramebuffer.h"
#include "rendering/resources/render/SceneRenderTypes.h"

class TemporalAAPass
{
public:
    TemporalAAPass(int width, int height, Camera& camera, SceneRenderConfig& config);

    void beginFrame(int width, int height);
    GLuint resolve(int width, int height, Framebuffer& source, Screenquad& screenQuad,
                   Shader& shader);
    void resize(int width, int height);

private:
    Camera& camera_;
    SceneRenderConfig& config_;
    TemporalAAFramebuffer history_;
    glm::mat4 previousViewProjection_ = glm::mat4(1.0f);
    glm::mat4 currentViewProjection_ = glm::mat4(1.0f);
    bool historyValid_ = false;
    int writeIndex_ = 0;
    unsigned int frameIndex_ = 0;

    static float halton(unsigned int index, unsigned int base);
};
