#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform sampler2D skyColor;
uniform sampler2D skyBright;

void main()
{
    // Linear filtering performs the spatial reconstruction. Full-resolution
    // TAA then accumulates the existing blue-noise-jittered cloud samples.
    FragColor = texture(skyColor, TexCoords);
    BrightColor = texture(skyBright, TexCoords);
}
