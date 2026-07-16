const float TERRAIN_TWO_PI = 6.28318530717958647692;

// Single source of truth for all terrain biome consumers. TDM channels are
// height, slope, aspect and curvature; no downstream stage re-derives them.
vec3 biomeWeights(float h, float aspect, float n)
{
    float sunFacing = cos(aspect * TERRAIN_TWO_PI - u_sunAzimuth);
    float shift = sunFacing * u_terrainSunHeightShift +
                  (n - 0.5) * u_terrainNoiseHeightShift;
    float shiftedHeight = h - shift;

    // Strict adjacent-band classification. Grass and snow can never mix
    // directly: grass -> rock is the lower transition and rock -> snow is the
    // upper transition. Outside those two bands exactly one material is 1.0.
    float grassToRock = smoothstep(u_grassEnd, u_rockStart, shiftedHeight);
    float rockToSnow = smoothstep(u_snowStart, u_snowEnd, shiftedHeight);
    float grass = 1.0 - grassToRock;
    float snow = grassToRock * rockToSnow;
    float rock = grassToRock * (1.0 - rockToSnow);
    return vec3(grass, rock, snow);
}
