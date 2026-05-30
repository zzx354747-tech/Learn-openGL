#version 330 core

in vec3 TexCoords;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform samplerCube skybox;
uniform vec3 cameraPos;
uniform bool isSkybox;
uniform bool uGammaCorrection;

void main()
{
    if (isSkybox)
    {
        FragColor = texture(skybox, TexCoords);
    }
    else
    {
        vec3 incident = normalize(FragPos - cameraPos);
        vec3 reflected = reflect(incident, normalize(Normal));
        FragColor = vec4(texture(skybox, reflected).rgb, 1.0);
    }
}
