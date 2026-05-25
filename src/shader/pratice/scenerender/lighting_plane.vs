#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;

    // Directional Shadow
    vec4 FragPosLightSpace;

    // Spot Shadow
    vec4 FragPosSpotLightSpace;

} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

// Directional Shadow
uniform mat4 lightSpaceMatrix;

// Spot Shadow
uniform mat4 spotLightSpaceMatrix;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);

    vs_out.FragPos = worldPos.xyz;

    vs_out.Normal =
        mat3(transpose(inverse(model))) * aNormal;

    vs_out.TexCoords = aTexCoords;

    // Directional Shadow
    vs_out.FragPosLightSpace =
        lightSpaceMatrix * worldPos;

    // Spot Shadow
    vs_out.FragPosSpotLightSpace =
        spotLightSpaceMatrix * worldPos;

    gl_Position =
        projection * view * worldPos;
}