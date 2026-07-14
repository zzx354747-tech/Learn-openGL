#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

const int SAMPLE_COUNT = 16;
uniform vec3 samples[SAMPLE_COUNT];
uniform mat4 projection;
uniform mat4 view;
uniform mat3 normalMatrix;

uniform float screenWidth;
uniform float screenHeight;

const float radius = 0.5;
const float bias = 0.025;

void main()
{
    vec2 noiseScale = vec2(screenWidth / 4.0, screenHeight / 4.0);

    vec3 fragPos = vec3(view * vec4(texture(gPosition, TexCoords).rgb, 1.0)); // 片段位置（观察空间）
    vec3 normal = normalize(normalMatrix * texture(gNormal, TexCoords).rgb); // 法线（观察空间）
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).rgb);

    // 创建观察空间TBN矩阵
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // 初始化环境光遮蔽值
    float occlusion = 0.0;
    // 遮蔽计算
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        // 将样本点从切线空间转换到观察空间
        vec3 samplePos = TBN * samples[i]; // 切线空间 -> 观察空间
        samplePos = fragPos + samplePos * radius; // 将样本点偏移到片段位置

        // 将样本点投影到屏幕空间
        // 寻找虚拟采样点在屏幕空间的深度
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w; // 透视除法
        // 将NDC坐标转换为纹理坐标[0, 1]
        vec2 sampleTexCoords = offset.xy * 0.5 + 0.5;

        // 获取样本点的深度值
        float sampleDepth = vec3(view * vec4(texture(gPosition, sampleTexCoords).rgb, 1.0)).z; // 样本点深度（观察空间）
        // 规定遮蔽影响范围
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        // 如果样本点在片段前面且在遮蔽范围内，则增加遮蔽值
        if (sampleDepth >= samplePos.z + bias)
            occlusion += rangeCheck;
    }
    occlusion = 1.0 - (occlusion / float(SAMPLE_COUNT)); // 归一化遮蔽值
    FragColor = occlusion;
}
