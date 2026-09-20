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

    float gridSpacing = 1.0;

    vec2 gridCoordinate =
        worldPosition.xz / gridSpacing;

    // 计算u,v以1像素长度为单位变化的单轴跨度
    vec2 coordinateDerivative =
        fwidth(gridCoordinate);

    // fract取小数，为锯齿波函数，abs取绝对值，最终变成三角波函数，转折点位于0,0.5,1...
    // 计算交点在uv坐标系下距离最近网格线的距离
    vec2 distanceToGridLine =
        abs(fract(gridCoordinate - 0.5) - 0.5);

    // 计算距离最近竖直/水平 网格线隔了多少屏幕像素
    vec2 pixelDistance =
        distanceToGridLine / coordinateDerivative;

    // 获得最近网格线距离的屏幕单位像素长度
    float nearestLine =
    min(
        pixelDistance.x,
        pixelDistance.y
    );

    // 生成平滑透明度
    float lineAlpha = 1.0 - clamp(nearestLine, 0.0, 1.0);

    if (lineAlpha < 0.1)
    {
        discard;
    }

    gl_FragDepth = depth;

    FragColor = vec4(
        0.45,
        0.45,
        0.45,
        lineAlpha
    );
}
