#version 330 core

in float PhotonEnergy;
layout (location = 0) out vec2 PhotonDensity;

uniform int densityChannel;

void main()
{
    vec2 offset = gl_PointCoord * 2.0 - 1.0;
    float radiusSquared = dot(offset, offset);
    if (radiusSquared > 1.0 || PhotonEnergy <= 0.0)
        discard;
    float kernel = exp(-radiusSquared * 3.2) * PhotonEnergy;
    PhotonDensity = densityChannel == 0
        ? vec2(kernel, 0.0) : vec2(0.0, kernel);
}
