#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

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
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, 800, 600);

    float vertices[] = {
    // positions          // colors           // texture coords

    // back face
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,   0.0f, 0.0f,

    // front face
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,   0.0f, 0.0f,

    // left face
    -0.5f,  0.5f,  0.5f,  1.0f, 0.5f, 0.0f,   1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.5f, 0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.2f, 0.8f, 0.2f,   0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.2f, 0.8f, 0.2f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.2f, 0.2f, 0.8f,   0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.5f, 0.0f,   1.0f, 0.0f,

    // right face
     0.5f,  0.5f,  0.5f,  0.9f, 0.3f, 0.3f,   1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.3f, 0.9f, 0.3f,   1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.3f, 0.3f, 0.9f,   0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.3f, 0.3f, 0.9f,   0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.9f, 0.9f, 0.3f,   0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.9f, 0.3f, 0.3f,   1.0f, 0.0f,

    // bottom face
    -0.5f, -0.5f, -0.5f,  0.7f, 0.1f, 0.1f,   0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.1f, 0.7f, 0.1f,   1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.1f, 0.1f, 0.7f,   1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.1f, 0.1f, 0.7f,   1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.7f, 0.7f, 0.1f,   0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.7f, 0.1f, 0.1f,   0.0f, 1.0f,

    // top face
    -0.5f,  0.5f, -0.5f,  0.1f, 0.7f, 0.7f,   0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.7f, 0.1f, 0.7f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.7f, 0.7f, 0.7f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.7f, 0.7f, 0.7f,   1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.3f, 0.6f, 0.9f,   0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.1f, 0.7f, 0.7f,   0.0f, 1.0f
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

    unsigned int VBO, VAO;
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

    unsigned int texture1, texture2;
    glGenTextures(1, &texture1);
    glGenTextures(1, &texture2);
    //激活纹理单元
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data1 = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    if (data1)
    {        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else    {
        std::cout << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(data1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data2 = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);
    if (data2)    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else    {
        std::cout << "Failed to load texture2" << std::endl;
    }
    stbi_image_free(data2);

    Shader shader("src/shader/texture.vs", "src/shader/texture.fs");
    shader.use();
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);
    shader.setMat4("model", glm::mat4(1.0f));
    shader.setMat4("view", glm::mat4(1.0f));
    shader.setMat4("projection", glm::mat4(1.0f));

    glEnable(GL_DEPTH_TEST);
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        //绑定纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glBindVertexArray(VAO);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));
        
        int screenwidth, screenheight;
        glfwGetFramebufferSize(window, &screenwidth, &screenheight);
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), float(screenwidth) / float(screenheight), 0.1f, 100.0f);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle) + (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(1.0f, 0.3f, 0.5f));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //删除资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}


