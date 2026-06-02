#version 330 core

in vec3 TexCoords;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

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
    }
    else
    {
        vec3 incident = normalize(FragPos - cameraPos);
        vec3 reflected = reflect(incident, normalize(Normal));
        vec3 color = texture(skybox, reflected).rgb;
        color = pow(color, vec3(2.2));
        FragColor = vec4(color, 1.0);
    }
}
