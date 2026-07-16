const int SHARED_MAX_STORM_HOLES = 7;
const mat3 SHARED_FBM_ROTATION = mat3(
     0.00,  0.80,  0.60,
    -0.80,  0.36, -0.48,
    -0.60, -0.48,  0.64);

float sharedHash31(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float sharedValueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(sharedHash31(i + vec3(0, 0, 0)), sharedHash31(i + vec3(1, 0, 0)), f.x),
            mix(sharedHash31(i + vec3(0, 1, 0)), sharedHash31(i + vec3(1, 1, 0)), f.x), f.y),
        mix(mix(sharedHash31(i + vec3(0, 0, 1)), sharedHash31(i + vec3(1, 0, 1)), f.x),
            mix(sharedHash31(i + vec3(0, 1, 1)), sharedHash31(i + vec3(1, 1, 1)), f.x), f.y),
        f.z);
}

float sharedBaseFbmFiltered(vec3 p, float filterLod)
{
    float sum = 0.0;
    float weightSum = 0.0;
    float amplitude = 0.52;
    for (int octave = 0; octave < 4; ++octave)
    {
        float octaveWeight = 1.0 - smoothstep(
            3.0, 4.0, float(octave) + filterLod);
        sum += sharedValueNoise(p) * amplitude * octaveWeight;
        weightSum += amplitude * octaveWeight;
        p = SHARED_FBM_ROTATION * p * 2.03 + vec3(7.1, 3.7, 5.9);
        amplitude *= 0.5;
    }
    return sum * (0.975 / max(weightSum, 0.001));
}

float sharedDetailFbm(vec3 p)
{
    float sum = 0.0;
    float amplitude = 0.52;
    for (int octave = 0; octave < 3; ++octave)
    {
        sum += sharedValueNoise(p) * amplitude;
        p = SHARED_FBM_ROTATION * p * 2.07 + vec3(5.3, 9.1, 2.7);
        amplitude *= 0.5;
    }
    return sum;
}

float sharedWeatherFbm(vec3 p)
{
    float lowFrequency = sharedValueNoise(p);
    float highFrequency = sharedValueNoise(
        SHARED_FBM_ROTATION * p * 2.03 + vec3(7.1, 3.7, 5.9));
    return lowFrequency * 0.67 + highFrequency * 0.33;
}

vec2 sharedWindDirection()
{
    float lengthSquared = dot(cloudWindDirection, cloudWindDirection);
    return lengthSquared > 0.0001
        ? cloudWindDirection * inversesqrt(lengthSquared)
        : vec2(1.0, 0.0);
}

float sharedStormHash11(float value)
{
    return fract(sin(value * 12.9898 + 31.416) * 43758.5453123);
}

vec2 sharedRotateHole(vec2 value, float angle)
{
    float sine = sin(angle);
    float cosine = cos(angle);
    return vec2(cosine * value.x + sine * value.y,
               -sine * value.x + cosine * value.y);
}

float sharedGeneratedStormHole(vec2 worldXZ, float worldY, int index)
{
    float key = float(stormHoleSeed) * 0.071 + float(index) * 17.137;
    float positionX = sharedStormHash11(key + 0.37);
    float positionZ = sharedStormHash11(key + 2.11);
    float sizeRandom = sharedStormHash11(key + 4.73);
    float aspectRandom = sharedStormHash11(key + 7.19);
    float rotationRandom = sharedStormHash11(key + 9.97);
    float minimumRadius = min(stormHoleMinRadius, stormHoleMaxRadius);
    float maximumRadius = max(stormHoleMinRadius, stormHoleMaxRadius);
    float radius = index == 0
        ? mix(max(minimumRadius, maximumRadius * 0.62), maximumRadius, sizeRandom)
        : (index < 5
            ? mix(max(minimumRadius, maximumRadius * 0.52),
                  maximumRadius * 0.88, sizeRandom)
            : mix(minimumRadius, maximumRadius, pow(sizeRandom, 1.35)));

    vec2 center = stormHoleAnchor;
    if (index > 0 && index < 5)
    {
        float clusterAngle = 6.28318530718 *
            (float(index) / 5.0 + positionX * 0.12);
        float clusterDistance = mix(
            maximumRadius * 1.35, maximumRadius * 4.50, positionZ);
        center += vec2(cos(clusterAngle), sin(clusterAngle)) * clusterDistance;
    }
    else if (index >= 5)
    {
        float clusterAngle = 6.28318530718 *
            (float(index) / float(max(stormHoleCount, 1)) + positionX * 0.12);
        float clusterDistance = mix(
            maximumRadius * 2.2, maximumRadius * 5.6, positionZ);
        center += vec2(cos(clusterAngle), sin(clusterAngle)) * clusterDistance;
    }

    float height01 = clamp(
        (worldY - cloudBaseHeight) / max(cloudThickness, 1.0), 0.0, 1.0);
    vec2 lean = sunDirection.xz / max(sunDirection.y, 0.08);
    vec2 axisCenter = center + lean *
        (worldY - (cloudBaseHeight + cloudThickness * 0.5));
    float axialNoise = sharedValueNoise(vec3(
        vec2(key * 0.013, key * 0.021),
        height01 * 2.7 + cloudEvolutionTime * 0.012));
    float axialRadius = mix(0.82, 1.12, axialNoise);
    float aspect = mix(0.68, 1.42, aspectRandom);
    vec2 ellipseScale = vec2(aspect, 1.0 / aspect);
    vec2 local = sharedRotateHole(
        worldXZ - axisCenter, rotationRandom * 6.28318530718) /
        max(radius * axialRadius, 1.0);
    float polarAngle = atan(local.y, local.x);
    float edgeWarp =
        sin(polarAngle * (3.0 + floor(sharedStormHash11(key + 18.29) * 4.0)) +
            sharedStormHash11(key + 24.13) * 6.28318530718) *
            mix(0.055, 0.145, sharedStormHash11(key + 30.31)) +
        sin(polarAngle * (7.0 + floor(sharedStormHash11(key + 21.71) * 5.0)) +
            sharedStormHash11(key + 27.59) * 6.28318530718) *
            mix(0.025, 0.080, sharedStormHash11(key + 33.47));
    float turbulentEdge = sharedValueNoise(vec3(
        worldXZ * 0.00072,
        key * 0.019 + height01 * 2.1 + cloudEvolutionTime * 0.009));
    float shapedDistance = length(local * ellipseScale) + edgeWarp +
                           (turbulentEdge - 0.5) * 0.24;
    float aperture = 1.0 - smoothstep(
        1.0 - clamp(stormHoleSoftness, 0.05, 0.80),
        1.0, shapedDistance);
    float interiorStructure = sharedValueNoise(vec3(
        worldXZ * 0.00135 + vec2(key * 0.007),
        height01 * 4.2 - cloudEvolutionTime * 0.016));
    return aperture * mix(0.78, 0.97, interiorStructure);
}

float sharedStormLightHole(vec3 worldPos)
{
    float mask = 0.0;
    for (int i = 0; i < SHARED_MAX_STORM_HOLES; ++i)
    {
        if (i >= stormHoleCount)
            break;
        mask = max(mask,
            sharedGeneratedStormHole(worldPos.xz, worldPos.y, i));
    }
    return mask * clamp(stormHoleStrength, 0.0, 1.0);
}

// Low-frequency density used by the optical-depth shadow map.  It is the
// exact macro/weather body of the visible density field, intentionally ending
// before high-frequency edge erosion.  Detail erosion is below the physical
// cloud-shadow penumbra and would only alias at an 8-16 metre texel footprint.
float sharedCloudCoarseDensity(vec3 worldPos)
{
    float height01 = (worldPos.y - cloudBaseHeight) /
                     max(cloudThickness, 1.0);
    if (height01 <= 0.0 || height01 >= 1.0)
        return 0.0;

    float type = clamp(cloudType, 0.0, 1.0);
    float verticalProfile = smoothstep(
        0.0, mix(0.055, 0.16, type), height01) *
        (1.0 - smoothstep(mix(0.54, 0.76, type), 1.0, height01));
    vec2 sampleXZ = worldPos.xz + cloudWindOffset +
        sharedWindDirection() * cloudWindShear * cloudThickness * height01;
    vec2 warpCoord = sampleXZ * (0.00017 * max(cloudScale, 0.01));
    vec2 domainWarp = vec2(
        sharedValueNoise(vec3(warpCoord, 3.7 + cloudEvolutionTime * 0.65)),
        sharedValueNoise(vec3(warpCoord + vec2(17.3, -9.1),
                              8.2 - cloudEvolutionTime * 0.47))) - 0.5;
    sampleXZ += domainWarp * (720.0 / max(cloudScale, 0.25));

    float macroScale = 0.00068 * max(cloudScale, 0.01);
    vec3 samplePos = vec3(
        sampleXZ * macroScale,
        height01 * mix(1.55, 2.85, type)).xzy;
    samplePos += vec3(cloudEvolutionTime * 0.035,
                      cloudEvolutionTime * 0.070,
                     -cloudEvolutionTime * 0.025);
    float weather = sharedWeatherFbm(vec3(
        sampleXZ * macroScale * 0.24,
        2.4 + cloudEvolutionTime * 0.18));
    float billow = sharedBaseFbmFiltered(samplePos, 0.0);
    float threshold = mix(0.72, 0.36, clamp(cloudCoverage, 0.0, 1.0));
    threshold += (0.5 - weather) * 0.24;
    threshold -= cloudAnvilAmount * type * 0.10 *
                 smoothstep(0.42, 0.82, height01);
    float coarseDensity = smoothstep(
        threshold - 0.055, threshold + 0.14, billow) *
        verticalProfile * cloudDensity;
    return coarseDensity * (1.0 - sharedStormLightHole(worldPos));
}

float sharedCloudDensity(vec3 worldPos)
{
    float height01 = (worldPos.y - cloudBaseHeight) /
                     max(cloudThickness, 1.0);
    if (height01 <= 0.0 || height01 >= 1.0)
        return 0.0;

    float type = clamp(cloudType, 0.0, 1.0);
    float verticalProfile = smoothstep(
        0.0, mix(0.055, 0.16, type), height01) *
        (1.0 - smoothstep(mix(0.54, 0.76, type), 1.0, height01));
    vec2 sampleXZ = worldPos.xz + cloudWindOffset +
        sharedWindDirection() * cloudWindShear * cloudThickness * height01;
    vec2 warpCoord = sampleXZ * (0.00017 * max(cloudScale, 0.01));
    vec2 domainWarp = vec2(
        sharedValueNoise(vec3(warpCoord, 3.7 + cloudEvolutionTime * 0.65)),
        sharedValueNoise(vec3(warpCoord + vec2(17.3, -9.1),
                              8.2 - cloudEvolutionTime * 0.47))) - 0.5;
    sampleXZ += domainWarp * (720.0 / max(cloudScale, 0.25));

    float macroScale = 0.00068 * max(cloudScale, 0.01);
    vec3 samplePos = vec3(
        sampleXZ * macroScale,
        height01 * mix(1.55, 2.85, type)).xzy;
    samplePos += vec3(cloudEvolutionTime * 0.035,
                      cloudEvolutionTime * 0.070,
                     -cloudEvolutionTime * 0.025);
    float weather = sharedWeatherFbm(vec3(
        sampleXZ * macroScale * 0.24,
        2.4 + cloudEvolutionTime * 0.18));
    // Density is a world-space physical field.  Camera-dependent LOD here
    // would make the same cloud acquire a different silhouette in its shadow
    // pass, and would make shadows swim as the camera moves.  Filtering is
    // performed by each consumer after evaluating this common density.
    float billow = sharedBaseFbmFiltered(samplePos, 0.0);
    float threshold = mix(0.72, 0.36, clamp(cloudCoverage, 0.0, 1.0));
    threshold += (0.5 - weather) * 0.24;
    threshold -= cloudAnvilAmount * type * 0.10 *
                 smoothstep(0.42, 0.82, height01);
    float coarseDensity = smoothstep(
        threshold - 0.055, threshold + 0.14, billow) *
        verticalProfile * cloudDensity;
    coarseDensity *= 1.0 - sharedStormLightHole(worldPos);
    if (coarseDensity <= 0.004)
        return 0.0;

    vec3 detailPosition = vec3(
        sampleXZ * macroScale, height01 * 2.45).xzy;
    detailPosition = detailPosition * max(cloudDetailScale, 0.1) +
        vec3(11.0, -4.0, 7.0) +
        vec3(cloudEvolutionTime * 0.11,
            -cloudEvolutionTime * 0.07,
             cloudEvolutionTime * 0.09);
    float detail = sharedDetailFbm(detailPosition);
    float erosionNoise = mix(
        detail, 1.0 - abs(detail * 2.0 - 1.0), 0.35);
    float edgeMask = 1.0 - clamp(coarseDensity, 0.0, 1.0);
    float erosion = (1.0 - erosionNoise) * cloudErosionStrength *
        mix(0.72, 1.22, height01) * (0.38 + 0.62 * edgeMask) *
        1.0;
    return max(coarseDensity - erosion * cloudDensity, 0.0);
}
