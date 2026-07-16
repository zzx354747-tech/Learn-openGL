#version 330 core

layout (location = 0) in vec2 sourceWorldXZ;
layout (location = 1) in float sourceLakeIndex;

uniform sampler2D terrainDataMap;
uniform sampler2D lakeDataMap;
uniform float terrainSize;
uniform float terrainBaseHeight;
uniform float terrainMountainHeight;
uniform vec3 sunDirection;
uniform vec2 waterWindDirection;
uniform float waterTime;
uniform float waterWaveAmplitude;
uniform float waterWavelengthScale;
uniform float opticalDisplacementScale;
uniform bool refractPhotons;
uniform vec4 causticBounds0;
uniform vec4 causticBounds1;
uniform float photonPointSize;

out float PhotonEnergy;

const float TWO_PI = 6.28318530718;

struct WaveDifferential
{
    float height;
    float dhdx;
    float dhdz;
};

vec2 rotateDirection(vec2 direction, float radians)
{
    float c = cos(radians);
    float s = sin(radians);
    return vec2(c * direction.x - s * direction.y,
                s * direction.x + c * direction.y);
}

void accumulateWave(inout WaveDifferential wave, vec2 worldXZ,
                    vec2 direction, float amplitude, float wavelength,
                    float speed, float phase)
{
    float k = TWO_PI / max(wavelength, 0.1);
    float theta = dot(direction, worldXZ) * k + speed * waterTime + phase;
    wave.height += amplitude * sin(theta);
    wave.dhdx += amplitude * cos(theta) * k * direction.x;
    wave.dhdz += amplitude * cos(theta) * k * direction.y;
}

WaveDifferential evaluateWaterWaves(vec2 worldXZ)
{
    vec2 wind = length(waterWindDirection) > 0.0001
        ? normalize(waterWindDirection) : vec2(1.0, 0.0);
    float scale = max(waterWavelengthScale, 0.1);
    WaveDifferential wave = WaveDifferential(0.0, 0.0, 0.0);
    accumulateWave(wave, worldXZ, wind,
        waterWaveAmplitude, 18.0 * scale, 0.82, 0.0);
    accumulateWave(wave, worldXZ, rotateDirection(wind, 0.42),
        waterWaveAmplitude * 0.48, 10.0 * scale, 1.11, 1.7);
    accumulateWave(wave, worldXZ, rotateDirection(wind, -0.57),
        waterWaveAmplitude * 0.23, 6.2 * scale, 1.48, 3.1);
    accumulateWave(wave, worldXZ, rotateDirection(wind, 0.86),
        waterWaveAmplitude * 0.14, 3.8 * scale, 1.92, 5.4);
    return wave;
}

vec2 terrainUV(vec2 worldXZ)
{
    return worldXZ / max(terrainSize, 1.0) + 0.5;
}

float terrainHeight(vec2 worldXZ)
{
    float normalizedHeight = texture(terrainDataMap, terrainUV(worldXZ)).r;
    return terrainBaseHeight + normalizedHeight * terrainMountainHeight;
}

void rejectPhoton()
{
    PhotonEnergy = 0.0;
    gl_PointSize = 1.0;
    gl_Position = vec4(2.0, 2.0, 0.0, 1.0);
}

void main()
{
    vec3 lakeData = texture(lakeDataMap, terrainUV(sourceWorldXZ)).rgb;
    if (lakeData.r <= 0.02 || lakeData.g <= 0.0 || lakeData.b <= 0.0)
    {
        rejectPhoton();
        return;
    }

    WaveDifferential wave = evaluateWaterWaves(sourceWorldXZ);
    vec3 surfaceNormal = refractPhotons
        ? normalize(vec3(-wave.dhdx * opticalDisplacementScale, 1.0,
                         -wave.dhdz * opticalDisplacementScale))
        : vec3(0.0, 1.0, 0.0);
    vec3 refractedRay = refract(normalize(sunDirection), surfaceNormal,
                                1.0 / 1.333);
    if (refractedRay.y >= -0.001)
    {
        rejectPhoton();
        return;
    }

    float surfaceY = lakeData.b + (refractPhotons ? wave.height : 0.0);
    vec2 hitXZ = sourceWorldXZ;
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        float floorY = terrainHeight(hitXZ);
        float travel = max(surfaceY - floorY, 0.0) /
                       max(-refractedRay.y, 0.05);
        hitXZ = sourceWorldXZ + refractedRay.xz * travel;
    }

    vec4 bounds = sourceLakeIndex < 0.5 ? causticBounds0 : causticBounds1;
    vec2 localUV = (hitXZ - bounds.xy) / max(bounds.zw - bounds.xy, vec2(1.0));
    if (any(lessThan(localUV, vec2(0.0))) ||
        any(greaterThan(localUV, vec2(1.0))))
    {
        rejectPhoton();
        return;
    }

    float atlasX = (localUV.x + (sourceLakeIndex < 0.5 ? 0.0 : 1.0)) * 0.5;
    vec2 atlasUV = vec2(atlasX, localUV.y);
    gl_Position = vec4(atlasUV * 2.0 - 1.0, 0.0, 1.0);
    gl_PointSize = photonPointSize;
    PhotonEnergy = smoothstep(0.25, 2.0, lakeData.r) *
                   smoothstep(0.5, 8.0, lakeData.g);
}
