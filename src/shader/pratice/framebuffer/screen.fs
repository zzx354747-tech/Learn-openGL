#version 330 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;

uniform float exposure;
uniform bool enableHdr;
uniform bool enableGamma;
uniform bool enableBloom;
uniform float bloomStrength;

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    // Bloom 叠加
    if (enableBloom)
        hdrColor += bloomColor * bloomStrength;

    vec3 result = hdrColor;

    // Tone mapping
    if (enableHdr)
        result = vec3(1.0) - exp(-result * exposure);

    // Gamma correction
    if (enableGamma)
        result = pow(result, vec3(1.0 / 2.2));

    FragColor = vec4(result, 1.0);
}