#version 330 core

in vec3 TexCoords;
in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

uniform samplerCube skybox;
uniform vec3 cameraPos;
uniform bool isSkybox;

void main()
{
    if (isSkybox)
    {
        vec3 color = texture(skybox, TexCoords).rgb;
        color = pow(color, vec3(2.2));
        FragColor = vec4(color, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
    else
    {
        vec3 incident = normalize(FragPos - cameraPos);
        vec3 reflected = reflect(incident, normalize(Normal));
        vec3 color = texture(skybox, reflected).rgb;
        color = pow(color, vec3(2.2));
        FragColor = vec4(color, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
