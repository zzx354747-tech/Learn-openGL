#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D currentColor;
uniform sampler2D historyColor;
uniform sampler2D currentDepth;
uniform sampler2D currentPosition;
uniform sampler2D currentVelocity;
uniform vec2 invResolution;
uniform vec2 currentJitterPixels;
uniform mat4 inverseCurrentViewProjection;
uniform mat4 previousViewProjection;
uniform mat4 inverseCurrentSkyViewProjection;
uniform mat4 previousSkyViewProjection;
uniform float historyWeight;
uniform float sharpness;
uniform bool historyValid;
uniform bool hasCurrentPosition;
uniform bool hasCurrentVelocity;

vec3 sampleCurrent(vec2 uv)
{
    return texture(currentColor, clamp(uv, vec2(0.0), vec2(1.0))).rgb;
}

void main()
{
    // projection[2].xy moves rasterized geometry by the negative jitter.
    // Resolve from that shifted location onto a stable, unjittered output
    // pixel grid before temporal accumulation. Otherwise rejected history
    // exposes the raw camera jitter as whole-frame shaking.
    vec2 currentUV = clamp(TexCoords - currentJitterPixels * invResolution,
                           vec2(0.0), vec2(1.0));
    vec3 current = sampleCurrent(currentUV);
    if (!historyValid)
    {
        FragColor = vec4(current, 1.0);
        return;
    }

    float depth = texture(currentDepth, currentUV).r;
    vec4 clip = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    bool background = depth >= 0.999999;
    vec4 previousClip;
    if (background)
    {
        // The sky has no geometric depth and must not acquire parallax from
        // camera translation. Reproject it with rotation-only matrices.
        vec4 skyDirection = inverseCurrentSkyViewProjection * clip;
        skyDirection /= max(abs(skyDirection.w), 1e-5);
        previousClip = previousSkyViewProjection * skyDirection;
    }
    else
    {
        vec4 world;
        if (hasCurrentPosition)
        {
            // Deferred lighting already owns an RGB32F world position. It is
            // substantially more stable for kilometre-scale geometry than
            // reconstructing position from a 24-bit non-linear depth value.
            world = vec4(texture(currentPosition, currentUV).rgb, 1.0);
        }
        else
        {
            world = inverseCurrentViewProjection * clip;
            world /= max(abs(world.w), 1e-5);
        }
        previousClip = previousViewProjection * world;
    }
    vec2 historyUV = previousClip.xy / max(previousClip.w, 1e-5) * 0.5 + 0.5;
    if (!background && hasCurrentVelocity)
        historyUV += texture(currentVelocity, currentUV).rg;

    vec3 neighborhoodMin = vec3(1e20);
    vec3 neighborhoodMax = vec3(-1e20);
    vec3 neighborhoodMean = vec3(0.0);
    vec3 neighborhoodMeanSquare = vec3(0.0);
    vec3 crossMean = vec3(0.0);
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec3 value = sampleCurrent(currentUV + vec2(x, y) * invResolution);
            neighborhoodMin = min(neighborhoodMin, value);
            neighborhoodMax = max(neighborhoodMax, value);
            neighborhoodMean += value;
            neighborhoodMeanSquare += value * value;
            if (abs(x) + abs(y) == 1)
                crossMean += value;
        }
    }
    neighborhoodMean /= 9.0;
    neighborhoodMeanSquare /= 9.0;
    crossMean *= 0.25;

    bool validUV = all(greaterThanEqual(historyUV, vec2(0.001))) &&
                   all(lessThanEqual(historyUV, vec2(0.999))) &&
                   previousClip.w > 0.0;
    vec3 history = texture(historyColor, clamp(historyUV, vec2(0.0), vec2(1.0))).rgb;

    // Variance clipping keeps useful sub-pixel history while rejecting stale
    // samples at silhouettes. It is less destructive than a fixed min/max box.
    vec3 variance = max(neighborhoodMeanSquare - neighborhoodMean * neighborhoodMean,
                        vec3(0.0));
    vec3 extent = max(sqrt(variance) * 1.25, vec3(0.012));
    history = clamp(history, neighborhoodMean - extent, neighborhoodMean + extent);

    float velocity = length(historyUV - TexCoords);
    float motionConfidence = exp(-velocity * 85.0);
    float luminanceDelta = abs(dot(history - current, vec3(0.2126, 0.7152, 0.0722)));
    float reactive = exp(-luminanceDelta * 2.5);
    float weight = validUV ? historyWeight * motionConfidence * reactive : 0.0;
    vec3 resolved = mix(current, history, clamp(weight, 0.0, 0.96));

    // Restore detail lost through repeated bilinear history reprojection. The
    // anti-ringing term backs off around strong HDR edges and specular peaks.
    vec3 detail = current - crossMean;
    float localContrast = dot(neighborhoodMax - neighborhoodMin,
                              vec3(0.2126, 0.7152, 0.0722));
    float antiRinging = 1.0 - smoothstep(0.25, 2.0, localContrast);
    resolved += detail * clamp(sharpness, 0.0, 1.0) * antiRinging;
    resolved = clamp(resolved, neighborhoodMin - vec3(0.012),
                      neighborhoodMax + vec3(0.012));
    FragColor = vec4(resolved, 1.0);
}
