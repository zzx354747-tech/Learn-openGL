#version 330 core

layout (location = 0) out float FragColor;

in vec2 TexCoords;

// X Pass: 原始 SSAO；Y Pass: X Pass 的结果
uniform sampler2D u_AOInput;

// 当前项目的 G-Buffer：
// gPosition.rgb = 世界空间位置
// gNormalRoughness.rgb = 世界空间法线
uniform sampler2D gPosition;
uniform sampler2D gNormalRoughness;

// 用于把世界空间位置转换到观察空间，比较观察空间 z
uniform mat4 view;

// X Pass: (1, 0)，Y Pass: (0, 1)
uniform vec2 u_Direction;

const int RADIUS = 2;

const float SPATIAL_WEIGHTS[5] = float[](
    0.60653066,
    0.88249690,
    1.00000000,
    0.88249690,
    0.60653066
);

// 观察空间单位下的深度容差，需要结合场景尺度调整
const float SIGMA_DEPTH = 0.1;

// 法线夹角敏感度
const float NORMAL_EXPONENT = 8.0;

const float EPSILON = 1e-6;

vec3 readNormal(vec2 uv)
{
    // gNormalRoughness.rgb 直接保存 [-1, 1] 的世界空间法线。
    vec3 normal = texture(gNormalRoughness, uv).rgb;
    float len2 = dot(normal, normal);

    if (len2 <= EPSILON)
        return vec3(0.0);

    return normal * inversesqrt(len2);
}

float readViewDepth(vec2 uv)
{
    vec3 worldPosition = texture(gPosition, uv).rgb;
    return (view * vec4(worldPosition, 1.0)).z;
}

void main()
{
    vec2 aoTexelSize = 1.0 / vec2(textureSize(u_AOInput, 0));
    float centerAO = texture(u_AOInput, TexCoords).r;

    vec3 centerNormal = readNormal(TexCoords);

    // G-Buffer 清屏为 0，背景没有有效法线。
    if (dot(centerNormal, centerNormal) <= EPSILON)
    {
        FragColor = centerAO;
        return;
    }

    float centerDepth = readViewDepth(TexCoords);
    float weightedAO = 0.0;
    float totalWeight = 0.0;

    for (int i = -RADIUS; i <= RADIUS; ++i)
    {
        // 避免屏幕边缘采样因默认 REPEAT 而绕到另一侧。
        vec2 sampleUV = clamp(
            TexCoords + u_Direction * float(i) * aoTexelSize,
            vec2(0.0),
            vec2(1.0)
        );

        vec3 sampleNormal = readNormal(sampleUV);
        if (dot(sampleNormal, sampleNormal) <= EPSILON)
            continue;

        float sampleAO = texture(u_AOInput, sampleUV).r;
        float sampleDepth = readViewDepth(sampleUV);

        float spatialWeight = SPATIAL_WEIGHTS[i + RADIUS];

        float depthDifference = abs(centerDepth - sampleDepth);
        float depthWeight = exp(
            -(depthDifference * depthDifference) /
            (2.0 * SIGMA_DEPTH * SIGMA_DEPTH)
        );

        // 世界空间法线经过刚体视图变换后点积不变，因此无需转换。
        float normalSimilarity = clamp(
            dot(centerNormal, sampleNormal),
            0.0,
            1.0
        );
        float normalWeight = pow(normalSimilarity, NORMAL_EXPONENT);

        float weight = spatialWeight * depthWeight * normalWeight;

        weightedAO += sampleAO * weight;
        totalWeight += weight;
    }

    FragColor = totalWeight > EPSILON
        ? clamp(weightedAO / totalWeight, 0.0, 1.0)
        : centerAO;
}
