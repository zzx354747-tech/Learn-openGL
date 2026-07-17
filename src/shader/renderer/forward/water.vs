#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D lakeDataMap;
uniform vec2 waterWindDirection;
uniform float waterWaveAmplitude;
uniform float waterWavelengthScale;
uniform float waterTime;

out vec3 WorldPos;
out vec3 WaterNormal;
out vec2 LakeUV;
out float SurfaceWaterLevel;

const float TWO_PI = 6.28318530718;

vec2 rotateDirection(vec2 direction, float radians)
{
    float c = cos(radians);
    float s = sin(radians);
    return vec2(c * direction.x - s * direction.y,
                s * direction.x + c * direction.y);
}

void accumulateWave(inout float height, inout vec2 gradient,
                    vec2 worldXZ, vec2 direction, float amplitude,
                    float wavelength, float speed, float phase)
{
    float k = TWO_PI / max(wavelength, 0.1);
    float theta = dot(direction, worldXZ) * k + speed * waterTime + phase;
    height += amplitude * sin(theta);
    gradient += direction * (amplitude * cos(theta) * k);
}

void evaluateWaterWaves(vec2 worldXZ, out float height, out vec2 gradient)
{
    vec2 wind = length(waterWindDirection) > 0.0001
        ? normalize(waterWindDirection) : vec2(1.0, 0.0);
    float scale = max(waterWavelengthScale, 0.1);
    height = 0.0;
    gradient = vec2(0.0);
    accumulateWave(height, gradient, worldXZ, wind,
                   waterWaveAmplitude, 18.0 * scale, 0.82, 0.0);
    accumulateWave(height, gradient, worldXZ, rotateDirection(wind, 0.42),
                   waterWaveAmplitude * 0.48, 10.0 * scale, 1.11, 1.7);
    accumulateWave(height, gradient, worldXZ, rotateDirection(wind, -0.57),
                   waterWaveAmplitude * 0.23, 6.2 * scale, 1.48, 3.1);
    accumulateWave(height, gradient, worldXZ, rotateDirection(wind, 0.86),
                   waterWaveAmplitude * 0.14, 3.8 * scale, 1.92, 5.4);
}

void main()
{
    vec3 localPosition = aPos;
    vec3 lakeData = texture(lakeDataMap, aTexCoords).rgb;
    localPosition.y = lakeData.b;
    float depthFade = smoothstep(0.15, 2.0, lakeData.r);
    float shoreFade = smoothstep(1.5, 12.0, lakeData.g);
    float waveFade = depthFade * shoreFade;
    float height;
    vec2 gradient;
    evaluateWaterWaves(localPosition.xz, height, gradient);
    localPosition.y += height * waveFade;
    gradient *= waveFade;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec4 world = model * vec4(localPosition, 1.0);
    WorldPos = world.xyz;
    WaterNormal = normalize(normalMatrix * vec3(-gradient.x, 1.0, -gradient.y));
    LakeUV = aTexCoords;
    SurfaceWaterLevel = lakeData.b;
    gl_Position = projection * view * world;
}
