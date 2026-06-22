#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in vec2 aTexCoords1;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec2 TexCoords1;
out mat3 TBN;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    TexCoords = aTexCoords;
    TexCoords1 = aTexCoords1;

    // 将切线空间的T、B、N转换到世界空间
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // 去除T基底的N分量，使T垂直于N
    // 误差主要来源于线性插值，我们不对N作处理，一是因为要选它当基底，而是因为错了也没关系
    T = normalize(T - dot(T, N) * N);
    // B由叉乘得到，保证了T、B、N三者的正交性
    B = cross(N, T);

    TBN = mat3(T, B, N);

    vec4 WorldPos = model * vec4(aPos, 1.0);
    FragPos = WorldPos.xyz;
    Normal = N;
    gl_Position = projection * view * WorldPos;

    // 把世界空间变换到切线空间
    mat3 invTBN = transpose(TBN); 

    // 将视点位置和片段位置转换到切线空间
    TangentViewPos = invTBN * cameraPos;
    TangentFragPos = invTBN * FragPos;
}
