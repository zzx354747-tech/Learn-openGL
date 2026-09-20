#include "EditScreenPass.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

void EditScreenPass::render(
    int bfwidth,
    int bfheight,
    Framebuffer& framebuffer
)
{

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),
    static_cast<float>(bfwidth) /
        static_cast<float>(bfheight),
    0.1f,
    100.0f
    );

    glm::mat4 viewProjection = projection * view;

    glm::mat4 inverseViewProjection = glm::inverse(projection * view);

    framebuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);

    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);

    GLboolean previousDepthWrite = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthWrite);

    GLboolean blendEnabled = glIsEnabled(GL_BLEND);

    GLint previousDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

    // 开启深度测试
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    // 设置深度比较规则
    glDepthFunc(GL_LESS);
    // 开启混合
    glEnable(GL_BLEND);
    // 设置最终颜色为 source * source alpha + destination * (1 - source alpha)
    glBlendEquation(GL_FUNC_ADD);
    // 设置混合系数，destination alpha = 1 - source alpha
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    editShader.use();

    editShader.setMat4("inverseViewProjection", inverseViewProjection);

    editShader.setMat4("viewProjection", viewProjection);

    screenQuad.draw();

    glDepthMask(previousDepthWrite);
    glDepthFunc(previousDepthFunc);


    if (depthTestEnabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    if (!blendEnabled)
    {
        glDisable(GL_BLEND);
    }

    framebuffer.unbind();
}
