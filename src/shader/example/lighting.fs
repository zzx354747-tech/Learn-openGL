#version 330 core

struct Material {
    // 漫反射贴图，决定颜色，但不决定强度
    sampler2D diffuse;
    // 材质对漫反射光RGB系数，同时决定颜色和强度
    sampler2D specular;
    // 高光点的散射程度，值越大，散射越小，光点越集中
    float shininess;        
};

struct Light {
    // 都是光自身的属性，和物体无关
    vec3 position;
    vec3 flashliPosition;
    // 环境光分量的RGB值，同时决定颜色和强度
    vec3 ambient;
    // 漫反射分量的RGB值，同时决定颜色和强度
    vec3 diffuse;
    // 镜面反射分量的RGB值，同时决定颜色和强度
    vec3 specular;

    // 衰减参数，常数项、一次项、二次项
    float constant;
    float linear;
    float quadratic;

    // 聚光灯方向
    vec3 direction;
    // 切光角
    float cutOff;
    // 外部切光角
    float outerCutOff;
};

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

out vec4 FragColor;

uniform Light light;
uniform Material material;
uniform vec3 viewPos;
uniform bool flashlightOn;

void main()
{
    float distance = length(light.position - FragPos);
    float flashlightDistance = length(light.flashliPosition - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance
    + light.quadratic * (distance * distance));
    float flashlightAttenuation = 1.0 / (light.constant + light.linear * flashlightDistance
    + light.quadratic * (flashlightDistance * flashlightDistance));
    // 定义环境光 （如果没有环境光，物体将完全黑暗，如果环境光过强，物体将过于明亮）
    vec3 ambient = light.ambient * vec3(texture(material.diffuse, TexCoords));  
    vec3 flashlightAmbient = light.ambient * vec3(texture(material.diffuse, TexCoords)) * 0.2;
    // 定义漫反射 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    vec3 flashlightDir = normalize(light.flashliPosition - FragPos);
    // 点乘计算夹角余弦值
    float diff = max(dot(norm, lightDir), 0.0);
    float spotdiff = max(dot(norm, flashlightDir), 0.0);
    // diff最大为1，所以matrial.d/light.d共同定义了最大漫反射强度
    vec3 diffuse = light.diffuse * (diff * vec3(texture(material.diffuse, TexCoords)));
    vec3 spotdiffuse = light.diffuse * (spotdiff * vec3(texture(material.diffuse, TexCoords)));

    // view direction
    vec3 viewDir = normalize(viewPos - FragPos);

    //reflection direction
    vec3 reflectionDir = reflect(-lightDir, norm);
    vec3 flashReflectionDir = reflect(-flashlightDir, norm);

    // specular 将dot后的值提升到material.shininess次幂
    // 意味着当dot趋近1时，光照才会显著增强，且shininess越大，高光点越集中
    float spec = pow(max(dot(viewDir, reflectionDir), 0.0), material.shininess);
    float spotSpec = pow(max(dot(viewDir, flashReflectionDir), 0.0), material.shininess);
    // spec最大为1，所以material.s/light.s共同定义了最大镜面反射强度
    vec3 specular = vec3(texture(material.specular, TexCoords)) * spec * light.specular;
    vec3 spotSpecular = vec3(texture(material.specular, TexCoords)) * spotSpec * light.specular;
    // 三部分都会影响最终颜色
    // 我们并没有定义物体本身颜色，而是定义了物体被看到的颜色

    float theta = dot(flashlightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    spotdiffuse *= intensity;
    spotSpecular *= intensity;

    // 距离衰减
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    flashlightAmbient *= flashlightAttenuation;
    spotdiffuse *= flashlightAttenuation;
    spotSpecular *= flashlightAttenuation;

    vec3 spotresult;

    if(flashlightOn)
    {
        spotresult = flashlightAmbient + spotdiffuse + spotSpecular;
    }
    else
    {
        spotresult = vec3(0.0);
    }
    
    vec3 lightresult = ambient + diffuse + specular;
    vec3 result = lightresult + spotresult;
    FragColor = vec4(result, 1.0);
}