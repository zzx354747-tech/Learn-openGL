#version 330 core

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;        
}

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform Material material;
uniform vec3 lightPos;
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // ambient
    vec3 ambient = lightColor * material.ambient; 

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    // material.diffuse物体反射光的能力，lightColor光源颜色，diff入射光与法线的夹角余弦值
    vec3 diffuse = lightColor * (diff * material.diffuse);

    // view direction
    vec3 viewDir = normalize(viewPos - FragPos);

    //reflection direction
    vec3 reflectionDir = reflect(-lightDir, norm);

    // specular
    float spec = pow(max(dot(viewDir, reflectionDir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}