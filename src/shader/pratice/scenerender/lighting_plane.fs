#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct Sun {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct FlashLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};

uniform sampler2D texture1;
uniform PointLight pointLight;
uniform Sun sun;
uniform FlashLight flashLight;
uniform vec3 viewPos;
uniform bool enablePointLight;
uniform bool enableDirectionalLight;
uniform bool enableFlashlight;

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    return light.ambient * baseColor + (light.diffuse * diff * baseColor + light.specular * spec) * attenuation;
}

vec3 calcSun(Sun light, vec3 normal, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    return light.ambient * baseColor + light.diffuse * diff * baseColor + light.specular * spec;
}

vec3 calcFlashLight(FlashLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    return light.ambient * baseColor + (light.diffuse * diff * baseColor + light.specular * spec) * attenuation * intensity;
}

void main()
{
    vec3 baseColor = texture(texture1, TexCoords).rgb;
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);
    if (enablePointLight)
        result += calcPointLight(pointLight, normal, FragPos, viewDir, baseColor);
    if (enableDirectionalLight)
        result += calcSun(sun, normal, viewDir, baseColor);
    if (enableFlashlight)
        result += calcFlashLight(flashLight, normal, FragPos, viewDir, baseColor);

    FragColor = vec4(result, 1.0);
}
