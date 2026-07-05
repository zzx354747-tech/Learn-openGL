#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isSkybox;

out vec3 TexCoords;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    if (isSkybox)
    {
        TexCoords = aPos;
        vec4 pos = projection * view * vec4(aPos, 1.0);
        gl_Position = pos.xyww;
    }
    else
    {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
}
