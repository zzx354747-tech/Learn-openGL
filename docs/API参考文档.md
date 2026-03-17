// ============================================================
// OpenGL / GLFW / GLAD API 查阅文档
// 参考自 test.cpp，按实际使用顺序排列
// ============================================================


// ============================================================
// 一、GLFW 初始化与配置
// ============================================================

// 初始化 GLFW 库，必须在所有 GLFW 函数之前调用
glfwInit();

// 配置 GLFW 窗口/上下文选项，如版本号、核心模式等
glfwWindowHint(int target, int hint);


// ============================================================
// 二、窗口创建与上下文
// ============================================================

// 创建窗口和 OpenGL 上下文，失败返回 NULL
GLFWwindow* window = glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share);

// 将指定窗口的 OpenGL 上下文设为当前线程上下文，必须在 gladLoadGL 之前调用
glfwMakeContextCurrent(GLFWwindow* window);

// 检查窗口是否应关闭，常用于主循环退出条件
glfwWindowShouldClose(GLFWwindow* window);

// 手动设置窗口关闭标志，true 表示应关闭
glfwSetWindowShouldClose(GLFWwindow* window, int value);

// 释放所有 GLFW 资源，程序结束时调用
glfwTerminate();


// ============================================================
// 三、GLAD 初始化
// ============================================================

// 加载所有 OpenGL 函数指针，必须在 glfwMakeContextCurrent 之后调用
gladLoadGL(GLADloadfunc load);

// GLFW 提供的函数指针获取函数，作为参数传给 gladLoadGL
glfwGetProcAddress(const char* name);


// ============================================================
// 四、视口与回调
// ============================================================

// 设置 OpenGL 渲染区域的位置和大小
glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// 注册窗口尺寸改变时的回调函数
glfwSetFramebufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun cbfun);


// ============================================================
// 五、顶点数组对象（VAO）
// ============================================================

// 生成顶点数组对象，记录顶点属性配置
glGenVertexArrays(GLsizei n, GLuint* arrays);

// 绑定 VAO，之后的顶点属性设置都会记录在此 VAO 中
glBindVertexArray(GLuint array);


// ============================================================
// 六、顶点缓冲对象（VBO）/ 索引缓冲对象（EBO）
// ============================================================

// 生成缓冲对象（VBO / EBO 通用）
glGenBuffers(GLsizei n, GLuint* buffers);

// 绑定缓冲对象到目标（GL_ARRAY_BUFFER = VBO，GL_ELEMENT_ARRAY_BUFFER = EBO）
glBindBuffer(GLenum target, GLuint buffer);

// 将数据写入当前绑定的缓冲对象（GL_STATIC_DRAW = 数据不变）
glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);


// ============================================================
// 七、着色器
// ============================================================

// 创建着色器对象（GL_VERTEX_SHADER 或 GL_FRAGMENT_SHADER）
glCreateShader(GLenum type);

// 设置着色器的 GLSL 源代码
glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length);

// 编译着色器源代码
glCompileShader(GLuint shader);

// 查询着色器的整型参数（如 GL_COMPILE_STATUS 检查编译结果）
glGetShaderiv(GLuint shader, GLenum pname, GLint* params);

// 获取着色器的编译错误日志信息
glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

// 删除着色器对象，链接到程序后即可释放
glDeleteShader(GLuint shader);


// ============================================================
// 八、着色器程序
// ============================================================

// 创建一个着色器程序对象
glCreateProgram();

// 将着色器附加到程序对象
glAttachShader(GLuint program, GLuint shader);

// 链接所有附加的着色器，生成最终可执行程序
glLinkProgram(GLuint program);

// 查询程序的整型参数（如 GL_LINK_STATUS 检查链接结果）
glGetProgramiv(GLuint program, GLenum pname, GLint* params);

// 获取着色器程序的链接错误日志信息
glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei* length, GLchar* infoLog);

// 激活并使用指定的着色器程序
glUseProgram(GLuint program);


// ============================================================
// 九、顶点属性
// ============================================================

// 配置顶点属性指针，告诉 OpenGL 如何解析顶点数据
// 参数一（index）：属性位置，对应 layout(location = N)
// 参数二（size） ：每个顶点该属性的分量数（如 vec3 = 3）
// 参数三（type） ：数据类型，GL_FLOAT
// 参数四（normalized）：是否归一化，GL_FALSE
// 参数五（stride）：两个顶点间的字节跨度
// 参数六（pointer）：属性在缓冲中的偏移量
glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);

// 启用指定位置的顶点属性
glEnableVertexAttribArray(GLuint index);


// ============================================================
// 十、输入处理
// ============================================================

// 查询键盘按键状态，返回 GLFW_PRESS 或 GLFW_RELEASE
glfwGetKey(GLFWwindow* window, int key);


// ============================================================
// 十一、渲染相关
// ============================================================

// 设置清屏背景颜色（RGBA，值域 0.0f ~ 1.0f）
glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

// 清除指定缓冲区（GL_COLOR_BUFFER_BIT / GL_DEPTH_BUFFER_BIT 等）
glClear(GLbitfield mask);

// 按顶点数组顺序绘制图元（不使用索引）
// 参数一（mode）：图元类型，GL_TRIANGLES
// 参数二（first）：起始顶点索引
// 参数三（count）：顶点数量
glDrawArrays(GLenum mode, GLint first, GLsizei count);

// 按 EBO 中的索引顺序绘制图元（使用索引）
// 参数一（mode）：图元类型，GL_TRIANGLES
// 参数二（count）：索引数量
// 参数三（type）：索引数据类型，GL_UNSIGNED_INT
// 参数四（indices）：索引数组偏移量，使用 EBO 时传 0
glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices);

// 交换前后缓冲区，将渲染结果显示到屏幕（双缓冲机制）
glfwSwapBuffers(GLFWwindow* window);

// 检查并处理所有待处理事件（键盘、鼠标、窗口等），每帧必须调用
glfwPollEvents();


// 配置 GLFW 窗口/上下文的选项
// 参数一（int target）：配置项名称，如 GLFW_CONTEXT_VERSION_MAJOR
// 参数二（int hint）  ：配置项的值
// 常用配置项：
//   GLFW_CONTEXT_VERSION_MAJOR  —— OpenGL 主版本号（如 3）
//   GLFW_CONTEXT_VERSION_MINOR  —— OpenGL 次版本号（如 3）
//   GLFW_OPENGL_PROFILE         —— 使用核心模式 GLFW_OPENGL_CORE_PROFILE
//   GLFW_OPENGL_FORWARD_COMPAT  —— macOS 必须设为 GL_TRUE
glfwWindowHint(int target, int hint);


// ============================================================
// 二、窗口创建与上下文
// ============================================================

// 创建一个窗口和 OpenGL 上下文
// 参数一（int width）        ：窗口宽度（像素）
// 参数二（int height）       ：窗口高度（像素）
// 参数三（const char* title）：窗口标题
// 参数四（GLFWmonitor* monitor）：全屏显示器，窗口模式传 NULL
// 参数五（GLFWwindow* share）：共享资源的窗口，不共享传 NULL
// 返回值：成功返回 GLFWwindow* 指针，失败返回 NULL
GLFWwindow* window = glfwCreateWindow(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share);

// 将指定窗口的 OpenGL 上下文设置为当前线程的上下文
// 参数（GLFWwindow* window）：目标窗口
// 必须在初始化 GLAD 之前调用
glfwMakeContextCurrent(GLFWwindow* window);

// 检查窗口是否应该关闭（如用户点击关闭按钮）
// 参数（GLFWwindow* window）：目标窗口
// 返回值：应该关闭返回 true，否则返回 false
// 常用于主循环的退出条件
glfwWindowShouldClose(GLFWwindow* window);

// 手动设置窗口的关闭标志
// 参数一（GLFWwindow* window）：目标窗口
// 参数二（int value）         ：true 标记为应关闭，false 取消关闭
glfwSetWindowShouldClose(GLFWwindow* window, int value);

// 释放所有 GLFW 资源，程序结束或初始化失败时调用
glfwTerminate();


// ============================================================
// 三、GLAD 初始化
// ============================================================

// 初始化 GLAD，加载所有 OpenGL 函数指针
// 参数（GLADloadfunc load）：函数指针加载器，通常传入 glfwGetProcAddress
// 返回值：成功返回非零值，失败返回 0
// 必须在 glfwMakeContextCurrent 之后调用
gladLoadGL(GLADloadfunc load);

// GLFW 提供的函数指针获取函数，作为参数传给 gladLoadGL
// 参数（const char* name）：OpenGL 函数名
// 返回值：对应函数的指针
glfwGetProcAddress(const char* name);


// ============================================================
// 四、视口与回调
// ============================================================

// 设置 OpenGL 视口的位置和大小（渲染区域）
// 参数一（GLint x）     ：视口左下角 x 坐标
// 参数二（GLint y）     ：视口左下角 y 坐标
// 参数三（GLsizei width）：视口宽度
// 参数四（GLsizei height）：视口高度
glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

// 注册窗口大小改变时的回调函数
// 参数一（GLFWwindow* window）  ：目标窗口
// 参数二（GLFWframebuffersizefun cbfun）：回调函数指针
// 回调函数签名：void callback(GLFWwindow* window, int width, int height)
glfwSetFramebufferSizeCallback(GLFWwindow* window, GLFWframebuffersizefun cbfun);


// ============================================================
// 五、顶点缓冲对象（VBO）
// ============================================================

// 生成缓冲对象，并将 ID 存入指针
// 参数一（GLsizei n）   ：要生成的缓冲对象数量
// 参数二（GLuint* buffers）：存储生成的缓冲对象 ID 的数组
glGenBuffers(GLsizei n, GLuint* buffers);

// 将缓冲对象绑定到指定目标
// 参数一（GLenum target）：目标类型，GL_ARRAY_BUFFER 表示顶点数据
// 参数二（GLuint buffer）：缓冲对象的 ID
glBindBuffer(GLenum target, GLuint buffer);

// 将数据写入当前绑定的缓冲对象
// 参数一（GLenum target）：目标类型，与 glBindBuffer 一致
// 参数二（GLsizeiptr size）：数据大小（字节数），用 sizeof() 获取
// 参数三（const void* data）：数据指针（顶点数组）
// 参数四（GLenum usage）：数据使用方式
//   GL_STATIC_DRAW  —— 数据不变，绘制使用（最常用）
//   GL_DYNAMIC_DRAW —— 数据频繁改变
//   GL_STREAM_DRAW  —— 数据每帧改变
glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage);


// ============================================================
// 六、输入处理
// ============================================================

// 查询某个键的当前状态
// 参数一（GLFWwindow* window）：目标窗口
// 参数二（int key）           ：键值，如 GLFW_KEY_ESCAPE（ESC 键）
// 返回值：GLFW_PRESS（按下）或 GLFW_RELEASE（释放）
glfwGetKey(GLFWwindow* window, int key);


// ============================================================
// 七、渲染相关
// ============================================================

// 设置清除颜色（背景色），RGBA 格式，值域 0.0f ~ 1.0f
// 参数一（GLfloat red）  ：红色分量
// 参数二（GLfloat green）：绿色分量
// 参数三（GLfloat blue） ：蓝色分量
// 参数四（GLfloat alpha）：透明度分量
glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

// 清除指定缓冲区的内容
// 参数（GLbitfield mask）：要清除的缓冲区，可用位或组合多个
//   GL_COLOR_BUFFER_BIT   —— 颜色缓冲区
//   GL_DEPTH_BUFFER_BIT   —— 深度缓冲区
//   GL_STENCIL_BUFFER_BIT —— 模板缓冲区
glClear(GLbitfield mask);

// 交换前后缓冲区（双缓冲机制，将渲染结果显示到屏幕）
// 参数（GLFWwindow* window）：目标窗口
// 每帧渲染结束后调用
glfwSwapBuffers(GLFWwindow* window);

// 检查并处理所有待处理的事件（键盘、鼠标、窗口事件等）
// 非阻塞，立即返回
// 每帧都应调用，否则窗口会无响应
glfwPollEvents();
