#version 330 core

layout(location = 0) out vec4 FragColor;

uniform vec3 outlineColor;

void main()
{
    FragColor = vec4(outlineColor, 1.0);
}
