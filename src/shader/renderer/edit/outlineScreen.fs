#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D maskTexture;

uniform vec3 outlineColor;
uniform vec2 texelSize;
uniform float outlineWidth;

void main()
{
    // 当前像素的 Mask 值
    float center = texture(maskTexture, TexCoords).r;

    // 邻域最大 Mask 值
    float maxMaskValue = 0.0;

    for (int x = -5; x <= 5; ++x)
    {
        for (int y = -5; y <= 5; ++y)
        {
            vec2 offset = vec2(x, y);

            if (length(offset) > outlineWidth)
                continue;

            float texelMask = texture(
                maskTexture,
                TexCoords + offset * texelSize
            ).r;

            maxMaskValue = max(maxMaskValue, texelMask);
        }
    }

    // 排除物体自身，只保留向外膨胀的区域
    float outline = maxMaskValue * (1.0 - center);

    FragColor = vec4(outlineColor, outline);
}