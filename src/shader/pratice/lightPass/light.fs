#version 330 core

// 声明编号0和1，会用来在drawbuffer列表中查询对应颜色附件的地址
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;

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

// 接收阴影贴图
uniform sampler2D shadowMap;
uniform samplerCube depthCubeMap;
uniform sampler2D spotShadowMap;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform float farPlane;

uniform PointLight pointLight;
uniform Sun sun;
uniform FlashLight flashLight;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 spotLightSpaceMatrix;
uniform bool enablePointLight;
uniform bool enableDirectionalLight;
uniform bool enableFlashlight;

uniform float bloomThreshold;

float SpotShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords =
        fragPosLightSpace.xyz /
        fragPosLightSpace.w;

    projCoords =
        projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth =
        projCoords.z;

    float shadow = 0.0;

    float bias = 0.005;

    vec2 texelSize =
        1.0 /
        textureSize(spotShadowMap, 0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth =
                texture(
                    spotShadowMap,
                    projCoords.xy +
                    vec2(x, y) * texelSize
                ).r;

            shadow +=
                currentDepth - bias > pcfDepth
                ? 1.0
                : 0.0;
        }
    }

    shadow /= 9.0;

    return shadow;
}

float PointShadowCalculation(vec3 fragPos)
{
    vec3 fragToLight = fragPos - pointLight.position;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 0.03;

    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + viewDistance / farPlane) / 25.0;

    vec3 sampleOffsetDirections[20] = vec3[]
    (
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );

    for (int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(
            depthCubeMap,
            fragToLight + sampleOffsetDirections[i] * diskRadius
        ).r;

        closestDepth *= farPlane;

        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    shadow /= float(samples);

    return shadow;
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // 将片段位置从裁剪空间转换到[0,1]范围内，告诉去哪里采样阴影贴图
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

     // 处理视野外的片段，防止出现不正确的阴影
    if (projCoords.z > 1.0)
        return 0.0;

    // 获取当前片段在阴影贴图中的深度
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float shadow = 0.0;
    float bias = 0.005;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(
                shadowMap,
                projCoords.xy + vec2(x, y) * texelSize
            ).r;

            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }

    shadow /= 9.0;
    return shadow;
}

vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 baseColor, float specularStrength)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (
        light.constant +
        light.linear * distance +
        light.quadratic * distance * distance
    );

    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specular = light.specular * spec * specularStrength;

    float shadowStrength = 0.8;
    float shadow = PointShadowCalculation(fragPos);
    shadow *= shadowStrength;

    diffuse *= (1.0 - shadow);
    specular *= (1.0 - shadow);

    return ambient + (diffuse + specular) * attenuation;
}

vec3 calcSun(Sun light, 
vec3 normal, 
vec3 viewDir, 
vec3 baseColor,
float specularStrength,
vec4 FragPosLightSpace)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specular = light.specular * spec * specularStrength;

    float shadowStrength = 0.8; // 阴影强度，可以根据需要调整

    float shadow = ShadowCalculation(FragPosLightSpace);

    shadow *= shadowStrength; // 将阴影强度应用到阴影值上

    // 将阴影应用到漫反射和镜面反射上
    diffuse *= (1.0 - shadow);
    specular *= (1.0 - shadow);
    
    return ambient + diffuse + specular;
}

vec3 calcFlashLight(FlashLight light, 
vec3 normal, 
vec3 fragPos, 
vec3 viewDir, 
vec3 baseColor,
float specularStrength,
vec4 FragPosSpotLightSpace)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = light.ambient * baseColor;
    vec3 diffuse = light.diffuse * diff * baseColor;
    vec3 specular = light.specular * spec * specularStrength;

    float shadow =
        SpotShadowCalculation(
            FragPosSpotLightSpace
        );

    float shadowStrength = 0.8;

    shadow *= shadowStrength;

    diffuse *= (1.0 - shadow);
    specular *= (1.0 - shadow);

    return ambient +
        (diffuse + specular)
        * attenuation
        * intensity;
}

void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 baseColor = texture(gAlbedoSpec, TexCoords).rgb;
    float specularStrength = texture(gAlbedoSpec, TexCoords).a;

    vec4 FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    vec4 FragPosSpotLightSpace = spotLightSpaceMatrix * vec4(FragPos, 1.0);

    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    if (enablePointLight)
        result += calcPointLight(pointLight, 
                    Normal, 
                    FragPos, 
                    viewDir, 
                    baseColor,
                    specularStrength);

    if (enableDirectionalLight)
        result += calcSun(sun, 
                    Normal, 
                    viewDir, 
                    baseColor,
                    specularStrength,
                    FragPosLightSpace);

    if (enableFlashlight)
        result += calcFlashLight(flashLight, 
                    Normal,
                    FragPos, 
                    viewDir,
                    baseColor,
                    specularStrength,
                    FragPosSpotLightSpace); 

    FragColor = vec4(result, 1.0);

    // 按人眼对三个颜色的敏感度计算亮度，如果亮度超过阈值，就把它写入BrightColor，否则写入黑色
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    
    if (brightness > bloomThreshold)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
