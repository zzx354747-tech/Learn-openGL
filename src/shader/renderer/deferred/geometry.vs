#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec2 aTexCoords1;   // 新增
layout (location = 4) in vec3 aTangent;
layout (location = 5) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec2 TexCoords1;   // 新增
out mat3 TBN;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform bool enableWind;
uniform float windTime;
uniform float windStrength;
uniform vec2 windDirection;

vec3 applyWind(vec3 position, float vertexWeight)
{
    if (!enableWind)
        return position;

    vec2 direction = normalize(windDirection);
    vec2 perpendicular = vec2(-direction.y, direction.x);
    float phase = dot(position.xz, vec2(0.071, 0.113));
    float slowWave = sin(windTime * 1.15 + phase);
    float gust = sin(windTime * 0.37 + phase * 0.41) * 0.5 + 0.5;
    float flutter = sin(windTime * 3.4 + phase * 2.7) * 0.18;
    vec2 localDirection = normalize(direction + perpendicular *
        sin(windTime * 0.53 + phase * 0.73) * 0.22);
    float bend = windStrength * (0.35 + slowWave * 0.35 + gust * 0.45 + flutter);
    float weight = vertexWeight * vertexWeight;
    position.xz += localDirection * bend * weight;
    position.y -= abs(bend) * 0.08 * weight;
    return position;
}

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    TexCoords = aTexCoords;
    TexCoords1 = aTexCoords1;   // 新增

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 B = normalize(normalMatrix * aBitangent);
    vec3 N = normalize(normalMatrix * aNormal);

    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);

    TBN = mat3(T, B, N);

    vec3 animatedPosition = applyWind(aPos, clamp(aTexCoords.y, 0.0, 1.0));
    vec4 WorldPos = model * vec4(animatedPosition, 1.0);
    FragPos = WorldPos.xyz;
    Normal = N;
    gl_Position = projection * view * WorldPos;

    mat3 invTBN = transpose(TBN);
    TangentViewPos = invTBN * cameraPos;
    TangentFragPos = invTBN * FragPos;
}
