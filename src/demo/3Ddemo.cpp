#define STB_IMAGE_IMPLEMENTATION    
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "scene/camera.h"
#include "core/shader.h"

Camera camera;

bool cursorLocked = true; // 光标是否被锁定
bool gravePresslastFrame = false; // 上一帧是否按下了`键  

float currentFrame = 0.0f; // 当前帧的时间
float lastFrame = 0.0f; // 上一帧的时间

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);     
    //wasd控制相机移动
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

    //检测`键切换光标锁定状态
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
   

// 鼠标回调函数
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!cursorLocked) return; // 如果光标未锁定，忽略鼠标输入
    camera.ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //macOS系统需要添加以下代码
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    //创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "3D Demo", nullptr, nullptr);
    if (window == nullptr)    
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    //设置上下文和回调函数
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //注册鼠标回调函数
    glfwSetCursorPosCallback(window, mouse_callback);
    //隐藏并禁用鼠标光标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //加载OpenGL函数指针    
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

    int width, height, nrChannels;
    unsigned int texture1, texture2;
    glGenTextures(1, &texture1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data1 = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    if (data1)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(data1);

    glGenTextures(1, &texture2);
    stbi_set_flip_vertically_on_load(true); // 翻转图像
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data2 = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);
    if (data2)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cerr << "Failed to load texture2" << std::endl;
    }
    stbi_image_free(data2);

    int fbwidth, fbheight;
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);
    //视口设置
    glViewport(0, 0, fbwidth, fbheight);

    Shader shader("src/shader/3Ddemo.vs", "src/shader/3Ddemo.fs");
    shader.use();
    shader.setInt("texture1", 0);
    shader.setInt("texture2", 1);

    //开启深度测试
    glEnable(GL_DEPTH_TEST);

    //主循环
    while (!glfwWindowShouldClose(window))
    {        
        //计算时间差
        currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        //处理输入
        processInput(window, deltaTime);
        //渲染
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //绑定纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        //获取每帧长宽
        int bfwidth, bfheight;
        glfwGetFramebufferSize(window, &bfwidth, &bfheight);
        //创建观察矩阵
        glm::mat4 view = camera.GetViewMatrix();
        //创建投影矩阵
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)bfwidth / (float)bfheight, 0.1f, 100.0f);
        shader.use();
        //绑定VAO
        glBindVertexArray(VAO);
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        //绘制多个立方体
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            float time = glfwGetTime();
            //第三个参数，斜着旋转
            model = glm::rotate(model, glm::radians(angle) + time, glm::vec3(1.0f, 0.3f, 0.5f));
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        //交换缓冲区和轮询事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //清理资源
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glfwTerminate();
    return 0;
}
