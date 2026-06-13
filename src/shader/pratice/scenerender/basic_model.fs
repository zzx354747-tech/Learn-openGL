#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform sampler2D texture_diffuse1;

void main()
{
    vec3 color = pow(texture(texture_diffuse1, TexCoords).rgb, vec3(2.2));
    
    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}