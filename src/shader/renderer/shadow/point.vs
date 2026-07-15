#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform bool enableWind;
uniform float windTime;
uniform float windStrength;
uniform vec2 windDirection;

vec3 applyWind(vec3 position, float vertexWeight)
{
    if (!enableWind) return position;
    vec2 direction = normalize(windDirection);
    vec2 perpendicular = vec2(-direction.y, direction.x);
    float phase = dot(position.xz, vec2(0.071, 0.113));
    float wave = sin(windTime * 1.15 + phase) * 0.35;
    float gust = (sin(windTime * 0.37 + phase * 0.41) * 0.5 + 0.5) * 0.45;
    float flutter = sin(windTime * 3.4 + phase * 2.7) * 0.18;
    vec2 localDirection = normalize(direction + perpendicular * sin(windTime * 0.53 + phase * 0.73) * 0.22);
    float bend = windStrength * (0.35 + wave + gust + flutter);
    float weight = vertexWeight * vertexWeight;
    position.xz += localDirection * bend * weight;
    position.y -= abs(bend) * 0.08 * weight;
    return position;
}

// 只负责变换到世界空间
// 顶点着色器一次只处理一个顶点
// 在这之后会发生图元装配,自动把三个顶点组成一个三角形,然后送到几何着色器
// 自定义变量也会被组装
void main()
{
    vec3 animatedPosition = applyWind(aPos, clamp(aTexCoords.y, 0.0, 1.0));
    gl_Position = model * vec4(animatedPosition, 1.0);
}
