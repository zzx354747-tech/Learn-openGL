#version 330 core

layout (location = 0) out float Transmittance;

uniform sampler2D opticalDepthTexture;
uniform vec2 inverseTextureSize;

void main()
{
    vec2 uv = gl_FragCoord.xy * inverseTextureSize;
    Transmittance = exp(-texture(opticalDepthTexture, uv).r);
}
