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

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
        processInput(window);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.use();
        lightingShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setVec3("objectColor", glm::vec3(1.0f, 0.5f, 0.31f));
        lightingShader.setVec3("viewPos", camera.Getposition());
        lightingShader.setVec3("lightPos", lightPos);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f));
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
        
        