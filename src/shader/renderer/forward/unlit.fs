#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform bool hasAlbedoMap;
uniform sampler2D albedoTexture;
uniform vec3 albedoColor;

void main()
{
    vec3 color = hasAlbedoMap
        ? texture(albedoTexture, TexCoords).rgb
        : albedoColor;

    color = pow(color, vec3(2.2));
    FragColor = vec4(color, 1.0);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
