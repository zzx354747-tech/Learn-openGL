#include "core/shader.h"
#include "scene/camera.h"
#include "rendering/assets/Mesh.h"
#include "rendering/assets/Model.h"
#include "rendering/core/Renderer.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

Camera camera;
FrameData frameData;
bool cursorLocked = true;
bool gravePressLastFrame = false;
bool flashLightOn = true;
bool flashLightToggleLastFrame = false;
float lastFrame = 0.0f;
float currentFrame = 0.0f;
glm::vec3 lightPos1(1.2f, 1.0f, 2.0f);
glm::vec3 lightPos2(2.0f, 1.0f, 0.0f);
glm::vec3 lightPos3(1.0f, 3.0f, 2.0f);
glm::vec3 lightPos4(1.0f, 0.0f, 2.0f);

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos){
    if (!cursorLocked) return;
    camera.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}
void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    bool gravePressCurrentFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    if (gravePressCurrentFrame && !gravePressLastFrame)
    {
        cursorLocked = !cursorLocked;
        if (cursorLocked)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera.Resetmouse();
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    gravePressLastFrame = gravePressCurrentFrame;

    bool flashLightToggleCurrentFrame = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (flashLightToggleCurrentFrame && !flashLightToggleLastFrame)
    {
        flashLightOn = !flashLightOn;
    }
    flashLightToggleLastFrame = flashLightToggleCurrentFrame;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Backpack Demo", nullptr, nullptr);
    if (!window)    
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int bfwidth, bfheight;
    glfwGetFramebufferSize(window, &bfwidth, &bfheight);

    glViewport(0, 0, bfwidth, bfheight);

    stbi_set_flip_vertically_on_load(true);
    Model backpack("../backpack/backpack.obj");

    Renderer renderer(&camera, &backpack);

    while (!glfwWindowShouldClose(window))
    {
        currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

       float time = static_cast<float>(glfwGetTime()) * 0.8f;

        // light 1: 大圆水平绕行
        lightPos1.x = sin(time) * 3.0f;
        lightPos1.y = 1.2f;
        lightPos1.z = cos(time) * 3.0f;
        frameData.lightPos[0] = lightPos1;

        // light 2: 慢速反向圆轨道
        lightPos2.x = cos(time * 0.7f) * 2.3f;
        lightPos2.y = 0.8f;
        lightPos2.z = sin(time * 0.7f) * 2.3f;
        frameData.lightPos[1] = lightPos2;

        // light 3: 小圆 + 上下起伏
        lightPos3.x = sin(time * 1.2f) * 1.8f;
        lightPos3.y = 1.8f + sin(time * 2.0f) * 1.0f;
        lightPos3.z = cos(time * 1.2f) * 1.8f;
        frameData.lightPos[2] = lightPos3;

        // light 4: 椭圆轨迹
        lightPos4.x = sin(time * 0.9f) * 3.5f;
        lightPos4.y = 0.6f + cos(time * 1.5f) * 0.8f;
        lightPos4.z = cos(time * 0.9f) * 1.5f;
        frameData.lightPos[3] = lightPos4;

        processInput(window, deltaTime);
        frameData.flashLightOn = flashLightOn;

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);
        frameData.bfwidth = bfwidth;
        frameData.bfheight = bfheight;

        renderer.render(frameData);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
    