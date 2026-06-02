#version 330 core

in vec3 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube skybox;

void main()
{    
    FragColor = texture(skybox, TexCoords);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}