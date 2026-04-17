#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "core/shader.h"
#include "scene/camera.h"
#include "stb_image.h"
#include "utils/uni_light.h"
#include "scene/light.h"
#include "utils/Graphics.h"

Camera camera;

bool cursorLocked = true;
bool gravePressLastFrame = false;
bool flashLightOn = true;
bool flashLightToggleLastFrame = false;
float lastFrame = 0.0f;
float currentFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
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

glm::vec3 lightPos1(1.2f, 1.0f, 2.0f);
glm::vec3 lightPos2(2.0f, 1.0f, 0.0f);
glm::vec3 lightPos3(1.0f, 3.0f, 2.0f);
glm::vec3 lightPos4(1.0f, 0.0f, 2.0f);
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
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Light Demo", nullptr, nullptr);
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

    VertexData vertices[36];
    float x =1.0f, y = 1.0f, z = 1.0f;
    setCubeAttribute(x, y, z);
    Cube(vertices);

    glm::vec3 cubePositions[10];
    setCubePosition(cubePositions);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, Normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, TexCoords));
    glEnableVertexAttribArray(2);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned int texture1;
    glGenTextures(1, &texture1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data1 = stbi_load("../container2.png", &width, &height, &nrChannels, 0);
    if (data1)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(data1);

    unsigned int texture2;
    glGenTextures(1, &texture2);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data2 = stbi_load("../c2_specular.png", &width, &height, &nrChannels, 0);
    if (data2)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture2" << std::endl;
    }
    stbi_image_free(data2);

    int bfwidth, bfheight;
    glfwGetFramebufferSize(window, &bfwidth, &bfheight);
    glViewport(0, 0, bfwidth, bfheight);

    Shader lightingCubeShader("../src/shader/pratice/lighting.vs", "../src/shader/pratice/lighting.fs");
    lightingCubeShader.use();
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(1.0f)));
    lightingCubeShader.setMat3("normalMatrix", normalMatrix);
    Shader lightCubeshader ("../src/shader/pratice/lighting.vs", "../src/shader/pratice/light_cube.fs");
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window, deltaTime);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);
        float time1 = glfwGetTime() * 0.2f;
        float time2 = glfwGetTime() * 0.3f;
        float radius = 2.0f;
        lightPos1.x = sin(time1) * radius;
        lightPos1.y = cos(time1) * radius;
        lightPos1.z = 2.0f;

        lightPos2.x = cos(time1) * radius;
        lightPos2.y = sin(time1) * radius;
        lightPos2.z = 2.0f;

        lightPos3.x = sin(time2) * radius;
        lightPos3.y = cos(time2) * radius;
        lightPos3.z = 2.0f;

        lightPos4.x = cos(time2) * radius;
        lightPos4.y = sin(time2) * radius;
        lightPos4.z = 2.0f;

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)bfwidth / (float)bfheight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 viewPos = camera.Getposition();
        lightingCubeShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        MaterialData material;
        material.diffuse = 0;
        material.specular = 1;
        material.shininess = 32.0f;
        
        SunLightData sun;
        sun.direction = glm::vec3(-0.2f, -1.0f, -0.3f);
        sun.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
        sun.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
        sun.specular = glm::vec3(0.5f, 0.5f, 0.5f);

        FlashLightData flash;   
        flash.position = camera.Getposition();
        flash.direction = camera.GetFront();
        flash.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
        flash.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
        flash.specular = glm::vec3(1.0f, 1.0f, 1.0f);
        flash.constant = 1.0f;
        flash.linear = 0.09f;
        flash.quadratic = 0.032f;
        flash.cutOff = glm::cos(glm::radians(12.5f));
        flash.outerCutOff = glm::cos(glm::radians(17.5f));

        PointLightData point1;
        point1.position = lightPos1;
        point1.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
        point1.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
        point1.specular = glm::vec3(1.0f, 1.0f, 1.0f);
        point1.constant = 1.0f;
        point1.linear = 0.09f;
        point1.quadratic = 0.032f;

        PointLightData point2;
        point2.position = lightPos2;
        point2.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
        point2.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
        point2.specular = glm::vec3(1.0f, 1.0f, 1.0f);
        point2.constant = 1.0f;
        point2.linear = 0.09f;
        point2.quadratic = 0.032f;

        PointLightData point3;
        point3.position = lightPos3;
        point3.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
        point3.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
        point3.specular = glm::vec3(1.0f, 1.0f, 1.0f);
        point3.constant = 1.0f;
        point3.linear = 0.09f;
        point3.quadratic = 0.032f;

        PointLightData point4;
        point4.position = lightPos4;
        point4.ambient = glm::vec3(0.05f, 0.05f, 0.05f);
        point4.diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
        point4.specular = glm::vec3(1.0f, 1.0f, 1.0f);
        point4.constant = 1.0f;
        point4.linear = 0.09f;
        point4.quadratic = 0.032f;

        setMaterial(lightingCubeShader, material);
        setFlashLight(lightingCubeShader, flash, flashLightOn);
        setSunLight(lightingCubeShader, sun);
        setPointLight(lightingCubeShader, 0, point1);
        setPointLight(lightingCubeShader, 1, point2);
        setPointLight(lightingCubeShader, 2, point3);
        setPointLight(lightingCubeShader, 3, point4);

        setUniLight(lightingCubeShader, viewPos, view, projection);

        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            lightingCubeShader.setMat4("model", model);
            lightingCubeShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        };

        lightCubeshader.use();
        lightCubeshader.setMat4("projection", projection);
        lightCubeshader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos1);
        model = glm::scale(model, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model2 = glm::mat4(1.0f);
        model2 = glm::translate(model2, lightPos2);
        model2 = glm::scale(model2, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model2);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model3 = glm::mat4(1.0f);
        model3 = glm::translate(model3, lightPos3);
        model3 = glm::scale(model3, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model3);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 model4 = glm::mat4(1.0f);
        model4 = glm::translate(model4, lightPos4);
        model4 = glm::scale(model4, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", model4);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);

    glfwTerminate();
    return 0;
}