#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;

// 只负责变换到世界空间
// 顶点着色器一次只处理一个顶点
// 在这之后会发生图元装配,自动把三个顶点组成一个三角形,然后送到几何着色器
// 自定义变量也会被组装
void main()
{
    gl_Position = model * vec4(aPos, 1.0);
}