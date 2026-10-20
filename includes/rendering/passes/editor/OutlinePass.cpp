#include "rendering/passes/editor/OutlinePass.h"
#include "rendering/core/ModelDrawer.h"
#include "rendering/core/SphereDrawer.h"
#include "rendering/uniforms/CameraUniformSetter.h"

#include <vector>

namespace
{
void restoreEnabled(GLenum capability, GLboolean enabled)
{
    if (enabled) glEnable(capability);
    else glDisable(capability);
}

// Save only context state: the mask pixels and owned shader uniforms are outputs.
// Scope guards also restore bindings if drawing exits through an exception.
struct OutlineStateGuard
{
    GLint drawFBO = 0, readFBO = 0, renderbuffer = 0;
    GLint viewport[4] = {};
    GLint program = 0, vao = 0, activeTexture = GL_TEXTURE0;
    GLint unpackBuffer = 0, sampler0 = 0, depthFunc = GL_LESS;
    GLint polygonMode[2] = {};
    GLboolean depthTest, depthWrite, stencilTest, scissorTest;
    GLboolean blend, cullFace, logicOp, rasterizerDiscard;
    GLboolean colorMask[4] = {};
    GLint srcRGB = 0, dstRGB = 0, srcAlpha = 0, dstAlpha = 0;
    GLint equationRGB = 0, equationAlpha = 0;
    std::vector<GLint> textures2D;

    explicit OutlineStateGuard(bool allTextureUnits = false)
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_SAMPLER_BINDING, &sampler0);
        glActiveTexture(activeTexture);
        glGetIntegerv(GL_POLYGON_MODE, polygonMode);
        depthTest = glIsEnabled(GL_DEPTH_TEST);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        stencilTest = glIsEnabled(GL_STENCIL_TEST);
        scissorTest = glIsEnabled(GL_SCISSOR_TEST);
        blend = glIsEnabledi(GL_BLEND, 0);
        cullFace = glIsEnabled(GL_CULL_FACE);
        logicOp = glIsEnabled(GL_COLOR_LOGIC_OP);
        rasterizerDiscard = glIsEnabled(GL_RASTERIZER_DISCARD);
        glGetBooleani_v(GL_COLOR_WRITEMASK, 0, colorMask);
        glGetIntegerv(GL_BLEND_SRC_RGB, &srcRGB);
        glGetIntegerv(GL_BLEND_DST_RGB, &dstRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &srcAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &dstAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRGB);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);

        // Model meshes can bind multiple material textures while writing a mask.
        GLint units = 1;
        if (allTextureUnits) glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &units);
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
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glUseProgram(program);
        glBindVertexArray(vao);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        for (std::size_t unit = 0; unit < textures2D.size(); ++unit)
        {
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
            glBindTexture(GL_TEXTURE_2D, textures2D[unit]);
        }
        glBindSampler(0, sampler0);
        glActiveTexture(activeTexture);
        restoreEnabled(GL_DEPTH_TEST, depthTest);
        glDepthFunc(depthFunc);
        glDepthMask(depthWrite);
        restoreEnabled(GL_STENCIL_TEST, stencilTest);
        restoreEnabled(GL_SCISSOR_TEST, scissorTest);
        restoreEnabled(GL_CULL_FACE, cullFace);
        restoreEnabled(GL_COLOR_LOGIC_OP, logicOp);
        restoreEnabled(GL_RASTERIZER_DISCARD, rasterizerDiscard);
        if (blend) glEnablei(GL_BLEND, 0);
        else glDisablei(GL_BLEND, 0);
        if (glBlendFuncSeparatei && glBlendEquationSeparatei)
        {
            glBlendFuncSeparatei(0, srcRGB, dstRGB, srcAlpha, dstAlpha);
            glBlendEquationSeparatei(0, equationRGB, equationAlpha);
        }
        else
        {
            glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
            glBlendEquationSeparate(equationRGB, equationAlpha);
        }
        glColorMaski(0, colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        glPolygonMode(GL_FRONT_AND_BACK, polygonMode[0]);
    }
};
}

OutlinePass::OutlinePass(Framebuffer& hdrfbo, GBuffer& gbuffer, Shader& maskShader,
                 Shader& outlineShader, Camera& camera,
                 Screenquad& screenquad, int width, int height)
    : maskShader(maskShader)
    , outlineShader(outlineShader)
    , camera(camera)
    , hdrfbo(hdrfbo)
    , gbuffer(gbuffer)
    , screenquad(screenquad)
{
    if (width <= 0 || height <= 0)
        return;

    OutlineStateGuard stateGuard;
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    if (maskRBO == 0)
        glGenRenderbuffers(1, &maskRBO);

    glBindRenderbuffer(GL_RENDERBUFFER, maskRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    if (maskFBO == 0)
        glGenFramebuffers(1, &maskFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, maskFBO);

    glFramebufferRenderbuffer(
    GL_FRAMEBUFFER,
    GL_DEPTH_STENCIL_ATTACHMENT,
    GL_RENDERBUFFER,
    maskRBO
    );

    if (maskTexture == 0)
        glGenTextures(1, &maskTexture);


    glBindTexture(GL_TEXTURE_2D, maskTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        width,
        height,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        maskTexture,
        0
    );

    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
        != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "OutlinePass: Framebuffer is not complete!"
                  << std::endl;
    }
    maskWidth = width;
    maskHeight = height;
}

void OutlinePass::render(const SceneObject* object,
                         int width, int height)
{
    if (!object || !object->isVisible || width <= 0 || height <= 0)
        return;

    if (maskFBO == 0 || maskTexture == 0 || maskRBO == 0)
        return;

    OutlineStateGuard stateGuard;
    resize(width, height);
    glDisable(GL_SCISSOR_TEST);

    glBindFramebuffer(
    GL_READ_FRAMEBUFFER,
    gbuffer.getFBO()
    );

    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        maskFBO
    );

    glBlitFramebuffer(
        0, 0, width, height,
        0, 0, width, height,
        GL_DEPTH_BUFFER_BIT,
        GL_NEAREST
    );

    // 第一阶段：生成物体 Mask
    renderMask(maskShader, object, width, height);

    // 第二阶段：渲染描边
    renderOutline(outlineShader, screenquad, width, height);
}

void OutlinePass::renderMask(Shader& maskShader, const SceneObject* object,
                int width, int height)
{
    if (!object || !object->isVisible || width <= 0 || height <= 0)
        return;

    if (maskFBO == 0 || maskTexture == 0 || maskRBO == 0)
        return;

    OutlineStateGuard stateGuard;
    resize(width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, maskFBO);
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisablei(GL_BLEND, 0);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_CULL_FACE);

    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    const GLfloat clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, clearColor);

    maskShader.use();
    CameraUniformSetter::apply(maskShader, camera, width, height);

    drawObject(maskShader, object);
}

void OutlinePass::renderOutline(Shader& outlineShader, Screenquad& screenquad,
                int width, int height)
{
    if (width <= 0 || height <= 0 || maskWidth <= 0 || maskHeight <= 0)
        return;

    OutlineStateGuard stateGuard;
    hdrfbo.bind();
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_RASTERIZER_DISCARD);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

   // 禁用深度测试，模版测试
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // 开启混合模式
    glEnablei(GL_BLEND, 0);
    if (glBlendFuncSeparatei && glBlendEquationSeparatei)
    {
        glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquationSeparatei(0, GL_FUNC_ADD, GL_FUNC_ADD);
    }
    else
    {
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    }

    outlineShader.use();
    outlineShader.setVec2(
    "texelSize",
    glm::vec2(1.0f / maskWidth, 1.0f / maskHeight)
    );

    // 设置描边颜色和宽度
    outlineShader.setVec3("outlineColor", glm::vec3(1.0f, 0.6f, 0.0f)); // 描边颜色
    outlineShader.setFloat("outlineWidth", 3.0f); // 描边宽度

    // 绑定 mask 纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, maskTexture);
    glBindSampler(0, 0);
    outlineShader.setInt("maskTexture", 0);

    screenquad.draw();
}

void OutlinePass::resize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    if (width == maskWidth && height == maskHeight)
        return;
    if (maskTexture == 0 || maskRBO == 0)
        return;

    maskWidth = width;
    maskHeight = height;

    OutlineStateGuard stateGuard;
    glActiveTexture(GL_TEXTURE0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, maskTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, maskWidth, maskHeight,
                 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    glBindRenderbuffer(GL_RENDERBUFFER, maskRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, maskWidth, maskHeight);
}

OutlinePass::~OutlinePass()
{
    if (maskRBO != 0)
        glDeleteRenderbuffers(1, &maskRBO);
    if (maskTexture != 0)
        glDeleteTextures(1, &maskTexture);
    if (maskFBO != 0)
        glDeleteFramebuffers(1, &maskFBO);
}

void OutlinePass::drawObject(Shader& shader, const SceneObject* object)
{
    if (!object || !object->isVisible)
        return;

    OutlineStateGuard stateGuard(true);
    const glm::mat4 modelMatrix = object->transform.matrix();
    shader.setMat4("model", modelMatrix);

    if (const auto* sphereRef = std::get_if<sphereRenderRef>(&object->ref))
    {
        if (sphereRef->drawer)
            sphereRef->drawer->drawPick(shader, modelMatrix);
    }
    else if (const auto* modelRef = std::get_if<ModelDrawer*>(&object->ref))
    {
        if (*modelRef)
            (*modelRef)->draw(shader, modelMatrix);
    }
}
