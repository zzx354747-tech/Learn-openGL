#version 330 core

uniform sampler2D scenetexture;

in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    FragColor = texture(scenetexture, TexCoords);
}