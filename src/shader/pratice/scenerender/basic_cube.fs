#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoords);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
