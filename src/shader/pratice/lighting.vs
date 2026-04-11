#version 330 core
// apos顶点坐标
layout (location = 0) in vec3 aPos;
// 插值发生在顶点着色器和片段着色器之间，所以我们需要将法线向量传递给片段着色器
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

void main()
{
    // 只取前3个分量，因为w分量需要参与前三项运算，而第四项在此时没有作用
    // 也可以取vec4，但在和lightPos运算时还是要取vec3
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    TexCoords = aTexCoord;
    // 内置输出变量gl_position
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}