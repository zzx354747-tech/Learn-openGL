#version 330 core

// 此为各向同性GGX，所以随意选择了局部坐标系的构建方式

out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main
{
    // 规定采样方向,终点为片段位置，起点为立方体中心
    // 构建局部坐标系
    vec3 N = normalize(localPos);
    vec3 up = abs(N.x) < 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    // 规定累加初始颜色
    vec3 irradiance = vec3(0.0);
    // 规定步长
    float sampleDelta = 0.025;
    // 采样次数
    float nrSamples = 0;

    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // 球面坐标转笛卡尔坐标
            vec3 tangentSample = vec3(sin(theta) * cos(phi), 
                            sin(theta) * sin(phi), cos(theta));
            // 将切线空间的采样向量转换为世界空间
            vec3 sampleVec = tangentSample.x * right + 
            tangentSample.y * up + tangentSample.z * N;

            irradiance += texture(environmentMap, 
            sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
   irradiance = PI * irradiance / float(nrSamples);

    FragColor = vec4(irradiance, 1.0);
}