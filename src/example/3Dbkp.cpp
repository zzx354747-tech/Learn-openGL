#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "scene/camera.h"
#include "core/shader.h"

Camera camera;

bool cursorLocked = true; // 光标是否被锁定
bool gravePresslastFrame = false; // 上一帧是否按下了`键

float lastFrame = 0.0f; // 上一帧的时间
float currentFrame = 0.0f; // 当前帧的时间

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
             camera.Resetmouse(); // 重置鼠标位置，避免突然跳转
         }
         else
         {
             glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
         }
     }
     gravePresslastFrame = gravePressedNow;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!cursorLocked) return; // 如果光标未锁定，忽略鼠标输入
    camera.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
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

    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Demo", nullptr, nullptr);
    if (window == nullptr)    
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
    // ---- 位置 ----       ---- 颜色 ----        ---- 纹理 ----
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,     0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,     1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,     1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,     1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,   1.0f, 0.0f, 0.0f,     0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 0.0f,     0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f,     1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,     1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,     1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 0.0f,     0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f,     0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,     1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 0.0f, 1.0f,     1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,     0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,   0.0f, 0.0f, 1.0f,     0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,   0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,     1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,     1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,   1.0f, 1.0f, 0.0f,     1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,     0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 1.0f, 0.0f,     0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 1.0f, 0.0f,     0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,     1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,     0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,     1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,     1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,     1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 1.0f,     0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,     0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,     0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,     1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,     1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,     1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,   0.0f, 1.0f, 1.0f,     0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,   0.0f, 1.0f, 1.0f,     0.0f, 1.0f
};

    glm::vec3 cubePositions[] = {
    glm::vec3( 0.0f,  0.0f,  0.0f), 
    glm::vec3( 2.0f,  5.0f, -15.0f), 
    glm::vec3(-1.5f, -2.2f, -2.5f),  
    glm::vec3(-3.8f, -2.0f, -12.3f),  
    glm::vec3( 2.4f, -0.4f, -3.5f),  
    glm::vec3(-1.7f,  3.0f, -7.5f),  
    glm::vec3( 1.3f, -2.0f, -2.5f),  
    glm::vec3( 1.5f,  2.0f, -2.5f), 
    glm::vec3( 1.5f,  0.2f, -1.5f), 
    glm::vec3(-1.3f,  1.0f, -1.5f)  
};


    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int lightVAO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    int width, height, nrChannels;
    unsigned int texture1, texture2;
    glGenTextures(1, &texture1);
    glActiveTexture(GL_TEXTURE0);
    stbi_set_flip_vertically_on_load(true); // 翻转图像
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data1 = stbi_load("../container.jpg", &width, &height, &nrChannels, 0);
    if (data1)
    {         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);
        stbi_image_free(data1);
    }
    else
    {
        std::cerr << "Failed to load texture1" << std::endl;
    }

    glGenTextures(1, &texture2);
    glActiveTexture(GL_TEXTURE1);
    stbi_set_flip_vertically_on_load(true); // 翻转图像
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data2 = stbi_load("../awesomeface.png", &width, &height, &nrChannels, 0);
    if (data2)
    {         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
        stbi_image_free(data2);
    }
    else
    {
        std::cerr << "Failed to load texture2" << std::endl;
    }

    int bfwidth, bfheight;
    glfwGetFramebufferSize(window, &bfwidth, &bfheight);
    glViewport(0, 0, bfwidth, bfheight);

    Shader shader("../src/shader/3Ddemo.vs", "../src/shader/3Ddemo.fs");
    shader.use();
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {        
        currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwGetFramebufferSize(window, &bfwidth, &bfheight);
        processInput(window, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)bfwidth / (float)bfheight, 0.1f, 100.0f);
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        glBindVertexArray(VAO);

        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            float time = glfwGetTime();
            model = glm::rotate(model, glm::radians(angle) + time, glm::vec3(1.0f, 0.3f, 0.5f));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        //检查循环事件
        glfwPollEvents();
        //交换缓冲区        
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);

    glfwTerminate();
    return 0;
}