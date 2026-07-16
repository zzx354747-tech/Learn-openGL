#version 330 core

in vec2 TexCoords;
layout (location = 0) out vec2 BlurredDensity;

uniform sampler2D sourceDensity;
uniform vec2 blurDirection;

vec2 atlasClamp(vec2 uv, float halfStart)
{
    vec2 texel = 1.0 / vec2(textureSize(sourceDensity, 0));
    uv.x = clamp(uv.x, halfStart + texel.x, halfStart + 0.5 - texel.x);
    uv.y = clamp(uv.y, texel.y, 1.0 - texel.y);
    return uv;
}

void main()
{
    float halfStart = TexCoords.x < 0.5 ? 0.0 : 0.5;
    vec2 result = texture(sourceDensity, TexCoords).rg * 0.227027;
    result += texture(sourceDensity,
        atlasClamp(TexCoords + blurDirection * 1.384615, halfStart)).rg * 0.316216;
    result += texture(sourceDensity,
        atlasClamp(TexCoords - blurDirection * 1.384615, halfStart)).rg * 0.316216;
    result += texture(sourceDensity,
        atlasClamp(TexCoords + blurDirection * 3.230769, halfStart)).rg * 0.070270;
    result += texture(sourceDensity,
        atlasClamp(TexCoords - blurDirection * 3.230769, halfStart)).rg * 0.070270;
    BlurredDensity = result;
}
