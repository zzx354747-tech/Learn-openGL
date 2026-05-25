#version 330 core

in vec4 FragPos;

// 光栅化之后，每个片段插值得到的世界坐标
uniform vec3 lightPos;
// 点光源阴影的最远距离
uniform float farPlane;

void main()
{
    // 计算距离
    float lightDistance = length(FragPos.xyz - lightPos);

    // 归一化,距离信息最终要写入深度缓冲，但深度缓冲范围是0-1
    lightDistance = lightDistance / farPlane;

    // 不用默认的深度值(z/w)，而是把距离信息写入深度缓冲
    // 最终生成的片段带有位置信息和距离信息
    // 但是位置信息只用来计算lightDistance
    gl_FragDepth = lightDistance;
}