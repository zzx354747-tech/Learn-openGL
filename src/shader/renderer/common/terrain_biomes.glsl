const float TERRAIN_TWO_PI = 6.28318530717958647692;

// Single source of truth for all terrain biome consumers. TDM channels are
// height, slope, aspect and curvature; no downstream stage re-derives them.
vec3 biomeWeights(float h, float slope, float aspect, float n)
{
    float sunFacing = cos(aspect * TERRAIN_TWO_PI - u_sunAzimuth);
    float shift = sunFacing * u_terrainSunHeightShift +
                  (n - 0.5) * u_terrainNoiseHeightShift;
    float shiftedHeight = h - shift;

    float snow = smoothstep(u_snowStart, u_snowEnd, shiftedHeight);
    float grass = 1.0 - smoothstep(u_grassEnd, u_rockStart, shiftedHeight);
    float rock = max(1.0 - snow - grass, 0.0);

    float steep = smoothstep(u_steepRockStart, u_steepRockEnd, slope);
    rock = max(rock, steep);
    grass *= 1.0 - steep;
    snow *= 1.0 - smoothstep(u_snowSlopeStart, u_snowSlopeEnd, slope);

    vec3 weights = max(vec3(grass, rock, snow), vec3(0.0));
    return weights / max(dot(weights, vec3(1.0)), 1e-4);
}
