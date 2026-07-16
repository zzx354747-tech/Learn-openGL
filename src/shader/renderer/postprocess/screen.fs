#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform bool enableHdr;
uniform bool enableGamma;
uniform bool enableBloom;
uniform float bloomStrength;
uniform float exposure;

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    if (enableBloom)
        hdrColor += texture(bloomBlur, TexCoords).rgb * bloomStrength;

    vec3 result = hdrColor;
    if (enableHdr)
        result = vec3(1.0) - exp(-result * exposure);
    if (enableGamma)
        result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}
