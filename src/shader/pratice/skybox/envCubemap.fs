#version 330 core

out vec4 FragColor;
in vec3 localPos;

uniform sampler2D equirectangularMap; 

const vec2 invAtan = vec2(0.15915494309, 0.31830988618);

vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));

    uv *= invAtan;
    uv += 0.5;

    return uv;
}

void main()
{
    vec3 dir = normalize(localPos);

    vec2 uv = SampleSphericalMap(dir);

    vec3 color = texture(equirectangularMap, uv).rgb;

    FragColor = vec4(color, 1.0);
}