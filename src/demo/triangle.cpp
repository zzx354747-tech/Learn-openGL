#include <iostream>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

//定义回调函数
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{    glViewport(0, 0, width, height);
}
//定义输入处理函数
void processInput(GLFWwindow* window)
{    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
} 

//创建顶点着色器
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos; 
void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";
//创建片段着色器
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main()
{
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)";
int main()
{
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
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //加载OpenGL函数指针
    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //设置视口
    glViewport(0, 0, 800, 600);

    float vertices[] = 
    {
        -0.5, -0.5 , 0.0,
         0.5, -0.5 , 0.0,
         0.0,  0.5 , 0.0
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    //绑定VAO
    glBindVertexArray(VAO);
    //绑定VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //将顶点数据复制到VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //解绑VBO和VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //创建顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    //将着色器源码附加到着色器对象上并编译
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    //检查编译是否成功
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    //创建片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    //创建着色器程序
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    //检查链接是否成功
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    //删除着色器对象
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //主循环
    while (!glfwWindowShouldClose(window))
    {       
        //处理输入
        processInput(window);

        //渲染指令
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //使用着色器程序
        glUseProgram(shaderProgram);
        //绑定VAO
        glBindVertexArray(VAO);
        //绘制三角形
        glDrawArrays(GL_TRIANGLES, 0, 3);

        //交换缓冲区和轮询IO事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //释放资源并退出
    glfwTerminate();
    return 0;
}