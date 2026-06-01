#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
    vec4 FragPosSpotLightSpace;
    mat3 TBN;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 lightSpaceMatrix;
uniform mat4 spotLightSpaceMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    // 将切线空间的T、B、N转换到世界空间
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // 去除T基底的N分量，使T垂直于N
    // 误差主要来源于线性插值，我们不对N作处理，一是因为要选它当基底，而是因为错了也没关系
    T = normalize(T - dot(T, N) * N);
    // B由叉乘得到，保证了T、B、N三者的正交性
    B = cross(N, T);

    vs_out.TBN = mat3(T, B, N);

    vs_out.FragPos = worldPos.xyz;
    vs_out.Normal = N;
    vs_out.TexCoords = aTexCoords;

    // 方向光阴影用
    vs_out.FragPosLightSpace =
        lightSpaceMatrix * worldPos;

    // flashlight / spotlight 阴影用
    vs_out.FragPosSpotLightSpace =
        spotLightSpaceMatrix * worldPos;

    gl_Position =
        projection * view * worldPos;
}