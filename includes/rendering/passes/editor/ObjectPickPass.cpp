#include "rendering/passes/editor/ObjectPickPass.h"
#include <stdexcept>

namespace
{
// Texture allocation must not change the caller's bindings or use its unpack PBO.
struct AllocationBindings
{
    GLint texture = 0;
    GLint renderbuffer = 0;
    GLint unpackBuffer = 0;

    AllocationBindings()
    {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    ~AllocationBindings()
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
    }
};

void restoreEnabled(GLenum capability, GLboolean enabled)
{
    if (enabled)
        glEnable(capability);
    else
        glDisable(capability);
}
}

ObjectPickPass::ObjectPickPass(int width, int height)
    : width(width), height(height)
{
    initResources();
}

ObjectPickPass::~ObjectPickPass()
{
    if (pickFBO != 0)
        glDeleteFramebuffers(1, &pickFBO);

    if (pickRBO != 0)
        glDeleteRenderbuffers(1, &pickRBO);

    if (pickTexture != 0)
        glDeleteTextures(1, &pickTexture);
}

void ObjectPickPass::initResources()
{
    AllocationBindings bindings;
    GLint previousDrawFBO = 0;
    GLint previousReadFBO = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFBO);
    // 1. 创建 FBO
    glGenFramebuffers(1, &pickFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pickFBO);

    // 2. 创建保存物体 ID 的纹理
    glGenTextures(1, &pickTexture);
    glBindTexture(GL_TEXTURE_2D, pickTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R32UI,
        width,
        height,
        0,
        GL_RED_INTEGER,
        GL_UNSIGNED_INT,
        nullptr
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        pickTexture,
        0
    );

    // 3. 创建深度 RBO
    glGenRenderbuffers(1, &pickRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, pickRBO);

    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH_COMPONENT24,
        width,
        height
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        pickRBO
    );

    // 4. 检查 FBO
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER)
        != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr
            << "Error: Pick framebuffer is not complete!"
            << std::endl;
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFBO);
}

void ObjectPickPass::begin()
{
    if (active)
        throw std::logic_error("ObjectPickPass::begin cannot be nested");

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedState.drawFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedState.readFBO);
    glGetIntegerv(GL_VIEWPORT, savedState.viewport);
    glGetIntegerv(GL_CURRENT_PROGRAM, &savedState.program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &savedState.vertexArray);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedState.activeTexture);

    // ModelDrawer currently binds material textures even for the picking shader.
    GLint textureUnits = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &textureUnits);
    savedState.textures2D.resize(textureUnits);
    for (GLint unit = 0; unit < textureUnits; ++unit)
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedState.textures2D[unit]);
    }
    glActiveTexture(savedState.activeTexture);

    savedState.depthTest = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &savedState.depthWrite);
    glGetIntegerv(GL_DEPTH_FUNC, &savedState.depthFunc);
    savedState.blend = glIsEnabledi(GL_BLEND, 0);
    savedState.stencilTest = glIsEnabled(GL_STENCIL_TEST);
    savedState.scissorTest = glIsEnabled(GL_SCISSOR_TEST);
    glGetBooleani_v(GL_COLOR_WRITEMASK, 0, savedState.colorMask);
    glGetIntegerv(GL_POLYGON_MODE, savedState.polygonMode);
    active = true;

    glBindFramebuffer(GL_FRAMEBUFFER, pickFBO);
    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisablei(GL_BLEND, 0);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    GLuint clearID[4] = { 0, 0, 0, 0 }; // 清除为 ID 0
    glClearBufferuiv(GL_COLOR, 0, clearID);
    const GLfloat clearDepth = 1.0f;
    glClearBufferfv(GL_DEPTH, 0, &clearDepth);
}

void ObjectPickPass::end()
{
    if (!active)
        return;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, savedState.drawFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, savedState.readFBO);
    glViewport(savedState.viewport[0], savedState.viewport[1],
               savedState.viewport[2], savedState.viewport[3]);
    glUseProgram(savedState.program);
    glBindVertexArray(savedState.vertexArray);

    for (std::size_t unit = 0; unit < savedState.textures2D.size(); ++unit)
    {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
        glBindTexture(GL_TEXTURE_2D, savedState.textures2D[unit]);
    }
    glActiveTexture(savedState.activeTexture);

    restoreEnabled(GL_DEPTH_TEST, savedState.depthTest);
    glDepthMask(savedState.depthWrite);
    glDepthFunc(savedState.depthFunc);
    if (savedState.blend)
        glEnablei(GL_BLEND, 0);
    else
        glDisablei(GL_BLEND, 0);
    restoreEnabled(GL_STENCIL_TEST, savedState.stencilTest);
    restoreEnabled(GL_SCISSOR_TEST, savedState.scissorTest);
    glColorMaski(0, savedState.colorMask[0], savedState.colorMask[1],
                 savedState.colorMask[2], savedState.colorMask[3]);
    glPolygonMode(GL_FRONT_AND_BACK, savedState.polygonMode[0]);
    active = false;
}

void ObjectPickPass::resize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0)
        return;
    if (width == newWidth && height == newHeight)
        return;

    width = newWidth;
    height = newHeight;

    AllocationBindings bindings;
    // 重新分配纹理和 RBO 的存储
    glBindTexture(GL_TEXTURE_2D, pickTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R32UI,
        width,
        height,
        0,
        GL_RED_INTEGER,
        GL_UNSIGNED_INT,
        nullptr
    );

    glBindRenderbuffer(GL_RENDERBUFFER, pickRBO);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH_COMPONENT24,
        width,
        height
    );

}

SceneObjectID ObjectPickPass::readID(int pixelX, int pixelY)
{
    if (pixelX < 0 || pixelX >= width || pixelY < 0 || pixelY >= height)
    {
        return InvalidSceneObjectID;
    }

    GLint previousFBO = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousFBO);

    // Read into a single CPU integer regardless of the caller's pixel-pack state.
    GLint previousPackBuffer = 0;
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);
    const GLenum packParameters[] = {
        GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH,
        GL_PACK_SKIP_PIXELS, GL_PACK_SKIP_ROWS, GL_PACK_SWAP_BYTES
    };
    GLint previousPackValues[5] = {};
    for (int i = 0; i < 5; ++i)
        glGetIntegerv(packParameters[i], &previousPackValues[i]);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ROW_LENGTH, 0);
    glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_PACK_SKIP_ROWS, 0);
    glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE);

    // 绑定用于读取的 FBO
    // GL_DRAW_FRAMEBUFFER	接收 GPU 绘制结果
    // GL_READ_FRAMEBUFFER	供读取像素、Blit 等操作使用
    // GL_FRAMEBUFFER	同时绑定为读取和绘制目标
    // 这里我们只需要读取像素，所以绑定为 GL_READ_FRAMEBUFFER
    glBindFramebuffer(GL_READ_FRAMEBUFFER, pickFBO);

    // 指定读取颜色附件0
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    GLuint objectID = 0;
    glReadPixels(
        pixelX,
        pixelY,
        1,
        1,
        GL_RED_INTEGER,
        GL_UNSIGNED_INT,
        &objectID
    );

    // 恢复之前的 FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousFBO);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, previousPackBuffer);
    for (int i = 0; i < 5; ++i)
        glPixelStorei(packParameters[i], previousPackValues[i]);

    return static_cast<SceneObjectID>(objectID);
}
