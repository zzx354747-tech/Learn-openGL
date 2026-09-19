#version 330 core

layout(location = 0) in vec2 aPos;

out vec3 nearPoint;
out vec3 farPoint;

uniform mat4 inverseViewProjection;

vec3 unproject(vec2 screenPos, float ndcDepth)
{
    vec4 clipSpacePos = vec4(screenPos, ndcDepth, 1.0);
    vec4 viewSpacePos = inverseViewProjection * clipSpacePos;
    return viewSpacePos.xyz / viewSpacePos.w;
}

void main()
{
    nearPoint = unproject(aPos, -1.0);
    farPoint = unproject(aPos, 1.0);

    gl_Position = vec4(aPos, 0.0, 1.0);
}