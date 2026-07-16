#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aTexCoords1;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool enableWind;
uniform float windTime;
uniform float windStrength;
uniform vec2 windDirection;

out vec2 TexCoords;
out vec2 TexCoords1;
out vec3 FragPos;
out vec3 Normal;

vec3 applyWind(vec3 p, float vertexWeight)
{
    if (!enableWind) return p;
    vec2 direction = normalize(windDirection);
    float phase = dot(p.xz, vec2(0.071, 0.113));
    float bend = windStrength * sin(windTime * 1.15 + phase) *
                 vertexWeight * vertexWeight;
    p.xz += direction * bend;
    return p;
}

void main()
{
    TexCoords = aTexCoords;
    TexCoords1 = aTexCoords1;
    vec3 animated = applyWind(aPos, clamp(aTexCoords.y, 0.0, 1.0));
    vec4 world = model * vec4(animated, 1.0);
    FragPos = world.xyz;
    Normal = normalize(transpose(inverse(mat3(model))) * aNormal);
    gl_Position = projection * view * world;
}
