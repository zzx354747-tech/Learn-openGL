#version 330 core
out vec4 FragColor;
in vec3 localPos;

// 此为各向同性GGX，所以随意选择了局部坐标系的构建方式

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;

// Van der Corput 序列，用于低差异采样
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersley 低差异序列
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

// GGX重要性采样
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    // 先插值再进行粗糙度线性映射和先进行粗糙度线性映射再进行插值结果是不一样的
    // 所以令a=roughness^2,先插值再进行粗糙度线性映射
    float a = roughness * roughness;

    // 计算能表达半程向量的方位角
    float phi = 2.0 * PI * Xi.x;
    // 计算能表达半程向量的天顶角
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // 将球面坐标转换为笛卡尔坐标
    // 使用球面坐标系表达出了半程向量的xyz值
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;       

    // 将半程向量从切线空间转换为世界空间
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);
    
    // 展开矩阵乘法
    return normalize(right * H.x + up * H.y + N * H.z);
}

void main()
{
    // 规定采样方向,终点为片段位置，起点为立方体中心
    // 构建局部坐标系
    vec3 N = normalize(localPos);
    // N=V假设，所以R=V=N
    vec3 V = N;
    vec3 R = V;

    // 设置采样次数 
    // const防止额外的性能开销
    // uint是因为Hammersley函数的参数是uint类型
    const uint SAMPLE_COUNT = 1024u;
    // 设置离散累加公式的分子初始状态
    vec3 prefilteredColor = vec3(0.0);
    // 设置离散累加公式的分母初始状态
    float totalWeight = 0.0;

    // 开始采样
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // 生成低差异采样点
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        // 生成半程向量
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        // 计算采样方向(入射方向Li)
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        // 计算点积N·L
        float NdotL = max(dot(N, L), 0.0);
        // 半程向量在正半球，并不意味着Li在正半球，所以需要判断N·L是否大于0
        if(NdotL > 0.0)
        {
            // 采样环境贴图
            vec3 sampleColor = texture(environmentMap, L).rgb;
            // 计算离散累加公式的分子
            prefilteredColor += sampleColor * NdotL;
            // 计算离散累加公式的分母
            totalWeight += NdotL;
        }
    }

    // 计算离散累加公式的最终结果
    prefilteredColor = prefilteredColor / totalWeight;
    FragColor = vec4(prefilteredColor, 1.0);
}