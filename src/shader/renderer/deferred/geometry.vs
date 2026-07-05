#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
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

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    TBN = mat3(T, B, N);

    vec4 WorldPos = model * vec4(aPos, 1.0);
    FragPos = WorldPos.xyz;
    Normal = N;
    gl_Position = projection * view * WorldPos;

    mat3 invTBN = transpose(TBN);
    TangentViewPos = invTBN * cameraPos;
    TangentFragPos = invTBN * FragPos;
}