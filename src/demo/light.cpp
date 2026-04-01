#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"
#include "camera.h"

Camera camera;
float lastFrame = 0.0f;
float currentFrame = 0.0f;
bool cursorLocked = true;
bool gravePresslastFrame = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
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

    bool gravePressedNow = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    if (gravePressedNow && !gravePresslastFrame)
    {        
        cursorLocked = !cursorLocked;
        if (cursorLocked)
        {            
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera.Resetmouse();
        }
    }
    gravePresslastFrame = gravePressedNow;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!cursorLocked) return; // 如果光标未锁定，忽略鼠标输入
    camera.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}

glm::vec3 lightPos(1.5f, 1.5f, 1.0f);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
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

    float vertices[] = {

    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,

    0.5f, -0.5f, -0.5f,     0.0f, 0.0f, -1.0f,

    0.5f, 0.5f, -0.5f,      0.0f, 0.0f, -1.0f,

    0.5f, 0.5f, -0.5f,      0.0f, 0.0f, -1.0f,

    -0.5f, 0.5f, -0.5f,     0.0f, 0.0f, -1.0f,

    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,



    -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f,

    0.5f, -0.5f, 0.5f,      0.0f, 0.0f, 1.0f,

    0.5f, 0.5f, 0.5f,       0.0f, 0.0f, 1.0f,

    0.5f, 0.5f, 0.5f,       0.0f, 0.0f, 1.0f,

    -0.5f, 0.5f, 0.5f,      0.0f, 0.0f, 1.0f,

    -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f,



    -0.5f, 0.5f, 0.5f,      -1.0f, 0.0f, 0.0f,

    -0.5f, 0.5f, -0.5f,     -1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f,     -1.0f, 0.0f, 0.0f,

    -0.5f, 0.5f, 0.5f,      -1.0f, 0.0f, 0.0f,



    0.5f, 0.5f, 0.5f,       1.0f, 0.0f, 0.0f,

    0.5f, 0.5f, -0.5f,      1.0f, 0.0f, 0.0f,

    0.5f, -0.5f, -0.5f,     1.0f, 0.0f, 0.0f,

    0.5f, -0.5f, -0.5f,     1.0f, 0.0f, 0.0f,

    0.5f, -0.5f, 0.5f,      1.0f, 0.0f, 0.0f,

    0.5f, 0.5f, 0.5f,       1.0f, 0.0f, 0.0f,



    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,

    0.5f, -0.5f, -0.5f,     0.0f, -1.0f, 0.0f,

    0.5f, -0.5f, 0.5f,      0.0f, -1.0f, 0.0f,

    0.5f, -0.5f, 0.5f,      0.0f, -1.0f, 0.0f,

    -0.5f, -0.5f, 0.5f,     0.0f, -1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,



    -0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f,

    0.5f, 0.5f, -0.5f,      0.0f, 1.0f, 0.0f,

    0.5f, 0.5f, 0.5f,       0.0f, 1.0f, 0.0f,

    0.5f, 0.5f, 0.5f,       0.0f, 1.0f, 0.0f,

    -0.5f, 0.5f, 0.5f,      0.0f, 1.0f, 0.0f,

    -0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f

    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int bfwidth, bfheight;
    glfwGetFramebufferSize(window, &bfwidth, &bfheight);
    glViewport(0, 0, bfwidth, bfheight);

    glEnable(GL_DEPTH_TEST);
    Shader lightingShader("../src/shader/shader.vs", "../src/shader/shader.fs");
    Shader lightcubShader("../src/shader/shader.vs", "../src/shader/light.fs");

    while (!glfwWindowShouldClose(window))
    {
        glfwGetFramebufferSize(window, &bfwidth, &bfheight);
        currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window, deltaTime);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time = glfwGetTime();
        float radius = 2.0f;
        lightPos.x = cos(time) * radius;
        lightPos.y = sin(time) * radius;
        //保持z轴不变，使光源在水平面上绕着原点旋转
        lightPos.z = 1.0f;

        lightingShader.use();
        lightingShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
        lightingShader.setVec3("viewPos", camera.Getposition());
        lightingShader.setVec3("lightPos", lightPos);
        lightingShader.setVec3("material.ambient",  glm::vec3(0.5f, 0.31f, 0.31f));
        lightingShader.setVec3("material.diffuse",  glm::vec3(1.0f, 0.5f, 0.31f));
        lightingShader.setVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
        lightingShader.setFloat("material.shininess", 32.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)bfwidth / (float)bfheight, 0.1f, 100.0f);
        lightingShader.setMat4("model", model);
        lightingShader.setMat4("view", view);
        lightingShader.setMat4("projection", projection);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        lightcubShader.use();
        lightcubShader.setMat4("model", glm::translate(glm::mat4(1.0f), lightPos) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)));
        lightcubShader.setMat4("view", view);
        lightcubShader.setMat4("projection", projection);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}
        
        