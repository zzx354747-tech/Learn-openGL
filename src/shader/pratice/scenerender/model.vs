#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    mat3 TBN;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
    vec4 FragPosSpotLightSpace;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform mat4 lightSpaceMatrix;
uniform mat4 spotLightSpaceMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    vs_out.FragPos = worldPos.xyz;
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 normal = normalize(normalMatrix * aNormal);
    vec3 tangent = normalize(normalMatrix * aTangent);
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    vec3 bitangent = normalize(normalMatrix * aBitangent);

    vs_out.Normal = normal;
    vs_out.TBN = mat3(tangent, bitangent, normal);
    vs_out.TexCoords = aTexCoords;

    vs_out.FragPosLightSpace =
        lightSpaceMatrix * worldPos;

    vs_out.FragPosSpotLightSpace =
        spotLightSpaceMatrix * worldPos;

    gl_Position =
        projection * view * worldPos;
}
