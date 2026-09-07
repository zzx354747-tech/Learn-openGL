#version 330 core

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D sceneColor;

void main()
{
    ivec2 texSize = textureSize(sceneColor, 0);
    vec2 texelSize = 1.0 / vec2(texSize);

    vec2 uv0 = TexCoords + vec2(-1.0, -1.0) * texelSize; // 左下
    vec2 uv1 = TexCoords + vec2( 1.0, -1.0) * texelSize; // 右下
    vec2 uv2 = TexCoords + vec2(-1.0,  1.0) * texelSize; // 左上
    vec2 uv3 = TexCoords + vec2( 1.0,  1.0) * texelSize; // 右上

    vec3 c0 = texture(sceneColor, uv0).rgb;
    vec3 c1 = texture(sceneColor, uv1).rgb;
    vec3 c2 = texture(sceneColor, uv2).rgb;
    vec3 c3 = texture(sceneColor, uv3).rgb;

    vec3 result = (c0 + c1 + c2 + c3) * 0.25;

    FragColor = vec4(result, 1.0);
}