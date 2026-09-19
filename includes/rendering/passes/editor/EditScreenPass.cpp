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

    GLint previousDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    // 设置深度比较规则
    glDepthFunc(GL_LESS);

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

    framebuffer.unbind();
}