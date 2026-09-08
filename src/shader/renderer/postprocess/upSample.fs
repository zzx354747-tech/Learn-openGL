#version 330 core

out vec4 FragColor;

uniform sampler2D uSourceTexture;
uniform float uFilterRadius;

in vec2 TexCoords;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(uSourceTexture, 0));
    vec2 d = texelSize * uFilterRadius;

    // 第一行（上方 3 点）
    vec3 a = texture(uSourceTexture, TexCoords + vec2(-d.x,  d.y)).rgb;
    vec3 b = texture(uSourceTexture, TexCoords + vec2( 0.0,  d.y)).rgb;
    vec3 c = texture(uSourceTexture, TexCoords + vec2( d.x,  d.y)).rgb;

    // 第二行（中间 3 点）
    vec3 d_ = texture(uSourceTexture, TexCoords + vec2(-d.x,  0.0)).rgb; // 改为 d_ 避免冲突
    vec3 e  = texture(uSourceTexture, TexCoords).rgb;                     // 正中心
    vec3 f  = texture(uSourceTexture, TexCoords + vec2( d.x,  0.0)).rgb;

    // 第三行（下方 3 点）
    vec3 g = texture(uSourceTexture, TexCoords + vec2(-d.x, -d.y)).rgb;
    vec3 h = texture(uSourceTexture, TexCoords + vec2( 0.0, -d.y)).rgb;
    vec3 i = texture(uSourceTexture, TexCoords + vec2( d.x, -d.y)).rgb;

    // 帐篷滤波加权累加到 result
    vec3 result = e * 4.0;
    result += (b + d_ + f + h) * 2.0;
    result += (a + c + g + i);
    result *= 0.0625; // 等价于 / 16.0

    // 输出计算后的颜色，Alpha 设为 1.0
    FragColor = vec4(result, 1.0);
}