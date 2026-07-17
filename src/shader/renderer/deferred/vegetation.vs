#version 330 core

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColorRoughness;
layout(location=3) in vec2 aWindVariation;
layout(location=4) in vec4 aUVMaterial;
layout(location=5) in vec4 iPosScale;
layout(location=6) in vec4 iRotColor;

out vec3 vWorldPosition;
out vec3 vNormal;
out vec4 vColorRoughness;
out vec2 vVelocity;
out vec4 vUVMaterial;
flat out int vPointMode;

uniform mat4 u_viewProjection;
uniform mat4 u_previousViewProjection;
uniform float u_time;
uniform float u_previousTime;
uniform float u_windSpeed;
uniform float u_windStrength;
uniform vec2 u_windDir;
uniform vec3 u_cameraPosition;
uniform bool u_pointMode;
uniform float u_pointWorldHeight;
uniform float u_pointPixelScale;
uniform float u_pointMaxPixels;
uniform float u_pointMinDistance;
uniform float u_pointMaxDistance;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

mat3 rotationY(float a)
{
    float c = cos(a), s = sin(a);
    return mat3(c,0,-s, 0,1,0, s,0,c);
}

mat3 tiltRotation(float a, float z)
{
    vec3 x = normalize(vec3(-sin(z), 0, cos(z)));
    float c = cos(a), s = sin(a);
    mat3 k = mat3(0,x.z,-x.y, -x.z,0,x.x, x.y,-x.x,0);
    return mat3(1) + s * k + (1 - c) * k * k;
}

vec3 shapeVariation(vec3 p)
{
    vec3 s = vec3(mix(.88,1.12,hash12(iPosScale.xz*.071)),
                  mix(.92,1.10,hash12(iPosScale.zx*.113+17)),
                  mix(.88,1.12,hash12(iPosScale.xz*.157+41)));
    return p * mix(vec3(1), s, aWindVariation.y);
}

vec2 windOffset(float t)
{
    vec2 d = length(u_windDir) > .0001 ? normalize(u_windDir) : vec2(1,0);
    float p = dot(iPosScale.xz, vec2(.13,.17)) + t * u_windSpeed;
    return d * (sin(p) + .5 * sin(p * 2.3)) * u_windStrength;
}

vec3 animatedWorld(float t)
{
    mat3 r = tiltRotation(iRotColor.y, iRotColor.z) * rotationY(iRotColor.x);
    vec3 w = iPosScale.xyz + r * (shapeVariation(aPosition) * iPosScale.w);
    vec2 o = windOffset(t) * aWindVariation.x * iPosScale.w;
    w.xz += o;
    w.y -= length(o) * .035 * aWindVariation.x;
    return w;
}

vec3 hueRotate(vec3 c, float a)
{
    const vec3 axis = vec3(.57735026919);
    return c*cos(a) + cross(axis,c)*sin(a) +
           axis*dot(axis,c)*(1-cos(a));
}

void main()
{
    uint packedValue = uint(iRotColor.w + .5);
    float hue = float(packedValue & 255u) / 255.;
    float value = float((packedValue >> 8u) & 255u) / 255. * .30 + .82;
    uint flags = (packedValue >> 23u) & 1u;
    vec3 color = hueRotate(aColorRoughness.rgb, (hue - .5) * .12) * value;
    if (flags != 0u)
        color = mix(color, vec3(.220,.145,.040), .58 * aWindVariation.y);
    vColorRoughness = vec4(color, aColorRoughness.a);
    vUVMaterial = aUVMaterial;

    if (u_pointMode)
    {
        // Far vegetation is a stable point sprite: no wind, no shape animation
        // and zero object velocity. Every instance remains loaded and receives
        // the same terrain depth test as geometric vegetation.
        vPointMode = 1;
        vNormal = vec3(0,1,0);
        vVelocity = vec2(0);
        vUVMaterial = vec4(0);
        vWorldPosition = iPosScale.xyz +
            vec3(0, u_pointWorldHeight * iPosScale.w * .48, 0);
        vec4 clip = u_viewProjection * vec4(vWorldPosition, 1);
        float distanceXZ = length(iPosScale.xz - u_cameraPosition.xz);
        if (distanceXZ < u_pointMinDistance ||
            distanceXZ > u_pointMaxDistance || clip.w <= 0.0)
        {
            gl_Position = vec4(2,2,2,1);
            gl_PointSize = 1.0;
            return;
        }
        gl_Position = clip;
        gl_PointSize = clamp(u_pointPixelScale *
            (u_pointWorldHeight * iPosScale.w) / max(clip.w, .001),
            1.0, u_pointMaxPixels);
        return;
    }

    vPointMode = 0;
    mat3 rotation = tiltRotation(iRotColor.y, iRotColor.z) * rotationY(iRotColor.x);
    vNormal = normalize(rotation * aNormal);
    vWorldPosition = animatedWorld(u_time);
    vec3 previousWorld = animatedWorld(u_previousTime);
    vec4 currentClip = u_viewProjection * vec4(vWorldPosition, 1);
    vec4 staticPreviousClip = u_previousViewProjection * vec4(vWorldPosition, 1);
    vec4 animatedPreviousClip = u_previousViewProjection * vec4(previousWorld, 1);
    vVelocity = (animatedPreviousClip.xy / max(animatedPreviousClip.w, 1e-5) -
                 staticPreviousClip.xy / max(staticPreviousClip.w, 1e-5)) * .5;
    gl_Position = currentClip;
}
