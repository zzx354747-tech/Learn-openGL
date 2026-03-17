#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include "shader1.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//定义回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{    glViewport(0, 0, width, height);
}
//定义输入处理函数
void processInput(GLFWwindow* window)
{    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main(){
    //初始化glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //macOS系统需要添加以下代码
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    //创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    //绑定窗口上下文
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //加载OpenGL函数指针
    if (!gladLoadGL(glfwGetProcAddress))
    {        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //设置视口
    glViewport(0, 0, 800, 600);

    //first triangle
    float vertices[] = {
        -0.5, -0.5 , 0.0,
         0.5, -0.5 , 0.0,
         0.5,  0.5 , 0.0,
    };

    //second triangle
    float vertices2[] = {
        -0.5, -0.5 , 0.0,
         0.5,  0.5 , 0.0,
        -0.5,  0.5 , 0.0,
    };

    unsigned int VBO1,VAO1,VBO2,VAO2;
    glGenVertexArrays(1, &VAO1);
    glGenBuffers(1, &VBO1);
    glBindVertexArray(VAO1);
    glBindBuffer(GL_ARRAY_BUFFER, VBO1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glBindVertexArray(VAO2);
    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices2), vertices2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    Shader1 shader("src/shader/db_triangle.vs", "src/shader/db_triangle1.fs", "src/shader/db_triangle2.fs");
    //主循环
    while (!glfwWindowShouldClose(window))
    {
        //输入
        processInput(window);

        //渲染
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //使用着色器程序1
        shader.use1();
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.0f));
        transform = glm::rotate(transform, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
        shader.setMat4_1("transform", transform);
        //绘制第一个三角形
        glBindVertexArray(VAO1);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        //使用着色器程序2
        shader.use2();
        glm::mat4 transform2 = glm::mat4(1.0f);
        shader.setMat4_2("transform", transform2);
        //绘制第二个三角形
        glBindVertexArray(VAO2);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        

        //交换缓冲区和轮询IO事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //删除资源
    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);
    glDeleteVertexArrays(1, &VAO2);
    glDeleteBuffers(1, &VBO2);
    //终止glfw，清理资源
    glfwTerminate();
    return 0;
}