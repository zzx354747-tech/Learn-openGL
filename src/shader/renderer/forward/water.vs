#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float waterTime;

out vec3 WorldPos;
out vec3 WaterNormal;
out vec2 WaterUV;

void main()
{
    vec3 p = aPos;
    vec2 d1 = normalize(vec2(0.82, 0.57));
    vec2 d2 = normalize(vec2(-0.38, 0.93));
    vec2 d3 = normalize(vec2(0.15, -0.99));

    float q1 = dot(p.xz, d1) * 0.52 + waterTime * 1.15;
    float q2 = dot(p.xz, d2) * 0.83 + waterTime * 1.72;
    float q3 = dot(p.xz, d3) * 1.31 + waterTime * 2.18;
    p.y += sin(q1) * 0.105 + sin(q2) * 0.052 + sin(q3) * 0.022;

    vec2 gradient = d1 * cos(q1) * 0.105 * 0.52 +
                    d2 * cos(q2) * 0.052 * 0.83 +
                    d3 * cos(q3) * 0.022 * 1.31;
    vec3 localNormal = normalize(vec3(-gradient.x, 1.0, -gradient.y));
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec4 world = model * vec4(p, 1.0);
    WorldPos = world.xyz;
    WaterNormal = normalize(normalMatrix * localNormal);
    WaterUV = aTexCoords;
    gl_Position = projection * view * world;
}
