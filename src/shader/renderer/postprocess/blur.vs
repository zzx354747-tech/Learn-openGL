#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    // 2D纹理，直接输出顶点位置
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}