#version 330 core

in vec2 TexCoords;

out vec3 FragColor;

uniform sampler2D sceneColor;
uniform float threshold;
uniform float knee;
uniform float maxBrightness;

void main()
{
    vec4 scene = texture(sceneColor, TexCoords);
    // Skybox writes alpha 0; opaque geometry and light visuals write alpha 1.
    // Write black rather than discard so the previous frame cannot leak through.
    if (scene.a <= 0.0)
    {
        FragColor = vec3(0.0);
        return;
    }
    vec3 color = scene.rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722)); // Calculate brightness using luminance formula

    float safeKnee = max(knee, 0.0001); // Avoid division by zero

    float edge0 = threshold - safeKnee;
    float edge1 = threshold + safeKnee;

    float weight = smoothstep(edge0, edge1, brightness);
    vec3 extracted = color * weight;
    // Preserve hue while bounding the energy sent into the blur pyramid.
    // Thresholding alone passes extreme specular peaks at their full HDR value.
    // The scene HDR texture remains untouched. Zero explicitly disables this cap.
    if (maxBrightness > 0.0)
    {
        float peak = max(max(extracted.r, extracted.g), extracted.b);
        extracted *= min(1.0, maxBrightness / max(peak, 0.0001));
    }
    FragColor = extracted;
}
