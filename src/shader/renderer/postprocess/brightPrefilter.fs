#version 330 core

in vec2 TexCoords;

out vec3 FragColor;

uniform sampler2D sceneColor;
uniform float threshold;
uniform float knee;

void main()
{
    vec3 color = texture(sceneColor, TexCoords).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722)); // Calculate brightness using luminance formula

    float safeKnee = max(knee, 0.0001); // Avoid division by zero

    float edge0 = threshold - safeKnee;
    float edge1 = threshold + safeKnee;

    float weight = smoothstep(edge0, edge1, brightness);
    FragColor = color * weight;
}