#version 330 core

in vec3 TexCoords;

layout (location = 0) out vec4 FragColor;

uniform samplerCube skybox;

void main()
{    
    vec3 color = pow(texture(skybox, TexCoords).rgb, vec3(2.2));

    FragColor = vec4(color, 1.0);
}
