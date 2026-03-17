//just VBO
float vertices[] = {
    -0.5f,  0.5f, 0.0f,  // 左
     0.5f,  0.5f, 0.0f,  // 右
     0.0f, -0.5f, 0.0f   // 下
};
//创建VBO
unsigned int VBO;
glGenBuffers(1, &VBO);
//绑定VBO
glBindBuffer(GL_ARRAY_BUFFER, VBO);
//将顶点数据复制到VBO中
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

while (!glfwWindowShouldClose(window)) {
    // 处理输入
    ...
    // 渲染
    ...
    //绑定VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //设置顶点属性指针
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //绘制三角形
    ...
}

//VAO and VBO
float vertices[] = {
    -0.5f,  0.5f, 0.0f,  // 左
     0.5f,  0.5f, 0.0f,  // 右
     0.0f, -0.5f, 0.0f   // 下
};


//创建VAO和VBO
unsigned int VBO, VAO;
glGenVertexArrays(1, &VAO);
glGenBuffers(1, &VBO);
//绑定VAO
glBindVertexArray(VAO);
//绑定VBO
glBindBuffer(GL_ARRAY_BUFFER, VBO);
//将顶点数据复制到VBO中
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//设置顶点属性指针
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

//解绑VBO
glBindBuffer(GL_ARRAY_BUFFER, 0);
//解绑VAO
glBindVertexArray(0);

while (!glfwWindowShouldClose(window)) {
    // 处理输入
    ...
    // 渲染
    ...
    //绑定VAO
    glBindVertexArray(VAO);
    //绘制三角形
    ...
}