#version 330 core

in vec3 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube skybox;

void main()
{    
    vec3 color = pow(texture(skybox, TexCoords).rgb, vec3(2.2));

    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}