#version 330 core

layout (location = 0) out float FilteredOpticalDepth;

uniform sampler2D sourceOpticalDepth;
uniform vec2 inverseTextureSize;

void main()
{
    vec2 uv = gl_FragCoord.xy * inverseTextureSize;
    const float kernel[3] = float[3](1.0, 2.0, 1.0);
    float tau = 0.0;
    float weightSum = 0.0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float weight = kernel[x + 1] * kernel[y + 1];
            tau += texture(sourceOpticalDepth,
                uv + vec2(x, y) * inverseTextureSize).r * weight;
            weightSum += weight;
        }
    }
    FilteredOpticalDepth = tau / weightSum;
}
