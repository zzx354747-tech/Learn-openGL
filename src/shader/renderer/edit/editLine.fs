#version 330 core

in vec3 nearPoint;
in vec3 farPoint;

out vec4 FragColor;

uniform mat4 viewProjection;

float calculateDepth(vec3 worldPosition)
{
    vec4 clipSpacePosition = viewProjection * vec4(worldPosition, 1.0);
    float ndcDepth = clipSpacePosition.z / clipSpacePosition.w;

    return (ndcDepth + 1.0) * 0.5; // Convert from [-1, 1] to [0, 1]
}

void main()
{
    vec3 rayDir = farPoint - nearPoint;

    if (abs(rayDir.y) < 0.00001) {
        discard; 
    }

    float t = -nearPoint.y / rayDir.y;

    if (t <= 0.0 || t > 1.0) {
        discard; 
    }

    vec3 worldPosition = nearPoint + t * rayDir;

    float depth = calculateDepth(worldPosition);

    if (depth > 1.0 || depth < 0.0) {
        discard; 
    }

    gl_FragDepth = depth;

    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
}