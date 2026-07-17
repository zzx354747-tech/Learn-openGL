#version 330 core

layout(location=0) out vec3 gPosition;
layout(location=1) out vec4 gNormalRoughness;
layout(location=2) out vec4 gAlbedoMetallic;
layout(location=3) out vec2 gVelocity;

in vec3 vWorldPosition;
in vec3 vNormal;
in vec4 vColorRoughness;
in vec2 vVelocity;
flat in int vPointMode;

void main()
{
    if (vPointMode != 0)
    {
        vec2 q = gl_PointCoord * 2.0 - 1.0;
        if (dot(q, q) > 1.0)
            discard;
    }

    vec3 n = normalize(vNormal);
    vec3 dx = dFdx(n), dy = dFdy(n);
    float variance = .5 * (dot(dx,dx) + dot(dy,dy));
    float roughness = sqrt(clamp(vColorRoughness.a * vColorRoughness.a +
                           min(variance,.24), .0016, 1));
    gPosition = vWorldPosition;
    gNormalRoughness = vec4(n, clamp(roughness,.04,1));
    gAlbedoMetallic = vec4(vColorRoughness.rgb, 0);
    gVelocity = vVelocity;
}
