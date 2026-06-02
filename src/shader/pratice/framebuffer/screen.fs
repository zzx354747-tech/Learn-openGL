#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float exposure;
uniform bool enableHdr;
uniform bool enableGamma;

void main()
{

    vec3 color = texture(screenTexture, TexCoords).rgb;
    if (enableHdr)
    {
        color = vec3(1.0) - exp(-color * exposure);
    }
    if (enableGamma)
    {
        color = pow(color, vec3(1.0/2.2));
    }
    
    FragColor = vec4(color, 1.0);
}