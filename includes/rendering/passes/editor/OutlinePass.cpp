#include "rendering/passes/editor/OutlinePass.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/uniforms/CameraUniformSetter.h"

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace
{
void restoreEnabled(GLenum capability, GLboolean enabled)
{
    if (enabled)
        glEnable(capability);
    else
        glDisable(capability);
}

struct StencilState
{
    GLint function = GL_ALWAYS;
    GLint reference = 0;
    GLint valueMask = 0;
    GLint writeMask = 0;
    GLint fail = GL_KEEP;
    GLint depthFail = GL_KEEP;
    GLint depthPass = GL_KEEP;

    void capture(bool back)
    {
        glGetIntegerv(back ? GL_STENCIL_BACK_FUNC : GL_STENCIL_FUNC, &function);
        glGetIntegerv(back ? GL_STENCIL_BACK_REF : GL_STENCIL_REF, &reference);
        glGetIntegerv(back ? GL_STENCIL_BACK_VALUE_MASK : GL_STENCIL_VALUE_MASK, &valueMask);
        glGetIntegerv(back ? GL_STENCIL_BACK_WRITEMASK : GL_STENCIL_WRITEMASK, &writeMask);
        glGetIntegerv(back ? GL_STENCIL_BACK_FAIL : GL_STENCIL_FAIL, &fail);
        glGetIntegerv(back ? GL_STENCIL_BACK_PASS_DEPTH_FAIL : GL_STENCIL_PASS_DEPTH_FAIL, &depthFail);
        glGetIntegerv(back ? GL_STENCIL_BACK_PASS_DEPTH_PASS : GL_STENCIL_PASS_DEPTH_PASS, &depthPass);
    }

    void restore(GLenum face) const
    {
        glStencilFuncSeparate(face, function, reference, static_cast<GLuint>(valueMask));
        glStencilMaskSeparate(face, static_cast<GLuint>(writeMask));
        glStencilOpSeparate(face, fail, depthFail, depthPass);
    }
};

// Restore on normal return and exception unwinding, including model texture binds.
struct OutlineStateGuard
{
    GLint drawFBO = 0, readFBO = 0;
    GLint viewport[4] = {};
    GLint program = 0, vertexArray = 0, activeTexture = GL_TEXTURE0;
    GLint polygonMode[2] = {};
    GLboolean depthTest = GL_FALSE, stencilTest = GL_FALSE;
    GLboolean depthWrite = GL_TRUE;
    GLint depthFunc = GL_LESS;
    GLboolean scissorTest = GL_FALSE, blend = GL_FALSE, logicOp = GL_FALSE;
    GLboolean colorMask[4] = {};
    StencilState front, back;
    std::vector<GLint> textures2D;

    OutlineStateGuard()
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vertexArray);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetIntegerv(GL_POLYGON_MODE, polygonMode);
        depthTest = glIsEnabled(GL_DEPTH_TEST);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
        stencilTest = glIsEnabled(GL_STENCIL_TEST);
        scissorTest = glIsEnabled(GL_SCISSOR_TEST);
        blend = glIsEnabledi(GL_BLEND, 0);
        logicOp = glIsEnabled(GL_COLOR_LOGIC_OP);
        glGetBooleani_v(GL_COLOR_WRITEMASK, 0, colorMask);
        front.capture(false);
        back.capture(true);

        GLint units = 0;
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &units);
        textures2D.resize(units);
        for (GLint unit = 0; unit < units; ++unit)
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &textures2D[unit]);
        }
        glActiveTexture(activeTexture);
    }

    ~OutlineStateGuard()
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glUseProgram(program);
        glBindVertexArray(vertexArray);
        for (std::size_t unit = 0; unit < textures2D.size(); ++unit)
        {
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
            glBindTexture(GL_TEXTURE_2D, textures2D[unit]);
        }
        glActiveTexture(activeTexture);
        front.restore(GL_FRONT);
        back.restore(GL_BACK);
        restoreEnabled(GL_DEPTH_TEST, depthTest);
        glDepthMask(depthWrite);
        glDepthFunc(depthFunc);
        restoreEnabled(GL_STENCIL_TEST, stencilTest);
        restoreEnabled(GL_SCISSOR_TEST, scissorTest);
        restoreEnabled(GL_COLOR_LOGIC_OP, logicOp);
        if (blend)
            glEnablei(GL_BLEND, 0);
        else
            glDisablei(GL_BLEND, 0);
        glColorMaski(0, colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
    }
};
}

OutlinePass::OutlinePass(Shader& shader, Camera& camera)
    : shader(shader), camera(camera)
{
}

void OutlinePass::render(Framebuffer& hdrfbo, const SceneObject* object,
                         int width, int height)
{
    if (!object || !object->isVisible || width <= 0 || height <= 0)
        return;

    OutlineStateGuard stateGuard;
    hdrfbo.bind();
    glViewport(0, 0, width, height);

    glEnable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisablei(GL_BLEND, 0);
    glDisable(GL_COLOR_LOGIC_OP);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Clear stencil marks, preserving the HDR color and scene depth.
    glStencilMask(0xFF);
    const GLint clearStencil = 0;
    glClearBufferiv(GL_STENCIL, 0, &clearStencil);

    // Mark the original silhouette without testing scene depth.
    glDisable(GL_DEPTH_TEST);
    glColorMaski(0, GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    shader.use();
    CameraUniformSetter::apply(shader, camera, width, height);
    shader.setVec3("outlineColor", glm::vec3(1.0f, 0.5f, 0.0f));

    const auto drawObject = [&](const glm::mat4& matrix)
    {
        shader.setMat4("model", matrix);
        if (const auto* sphereRef = std::get_if<sphereRenderRef>(&object->ref))
        {
            if (sphereRef->drawer)
                sphereRef->drawer->drawPick(shader, matrix);
        }
        else if (const auto* modelRef = std::get_if<ModelDrawer*>(&object->ref))
        {
            if (*modelRef)
                (*modelRef)->draw(shader, matrix);
        }
    };

    const glm::mat4 modelMatrix = object->transform.matrix();
    drawObject(modelMatrix);

    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0x00);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    // Occlude the outline against scene depth without changing that depth.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    drawObject(glm::scale(modelMatrix, glm::vec3(1.05f)));
}
