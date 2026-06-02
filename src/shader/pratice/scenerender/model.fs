#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    mat3 TBN;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
    vec4 FragPosSpotLightSpace;
} fs_in;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

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

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
// 接收阴影贴图
uniform sampler2D shadowMap;
uniform samplerCube depthCubeMap;
uniform sampler2D spotShadowMap;

uniform float farPlane;

uniform PointLight pointLight;
uniform Sun sun;
uniform FlashLight flashLight;
uniform vec3 viewPos;
uniform bool enablePointLight;
uniform bool enableDirectionalLight;
uniform bool enableFlashlight;
uniform bool hasNormalMap;
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
        // 立方体贴图，方向向量采样，标准方向+偏移方向*半径
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

    // 获取当前片段在阴影贴图中的深度
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // 计算阴影
    float shadow = 0.0f;
    float bias = 0.005; 
    // 计算单个片段偏移量
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    float pcfRadius = 2.5;
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize * pcfRadius).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }
    }
    shadow /= 25.0; // 取平均值

    float fadeStart = 0.75;
    float fadeEnd = 1.0;
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, projCoords.z);

shadow *= fade;

    if (projCoords.z > 1.0)
        shadow = 0.0;
            
    return shadow;
}

vec3 calcPointLight(PointLight light,
    vec3 normal,
    vec3 fragPos,
    vec3 viewDir,
    vec3 diffuseTex,
    vec3 specularTex
)
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

    vec3 ambient = light.ambient * diffuseTex;
    vec3 diffuse = light.diffuse * diff * diffuseTex;
    vec3 specular = light.specular * spec * specularTex;

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
        vec3 diffuseTex,
        vec3 specularTex)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 reflectDir = reflect(-lightDir, normal);
    float diff = max(dot(normal, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambient = light.ambient * diffuseTex;
    vec3 diffuse = light.diffuse * diff * diffuseTex;
    vec3 specular = light.specular * spec * specularTex;

    float shadowStrength = 0.8; // 阴影强度，可以根据需要调整

    float shadow = ShadowCalculation(fs_in.FragPosLightSpace);

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
        vec3 diffuseTex,
        vec3 specularTex)
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

    vec3 ambient = light.ambient * diffuseTex;
    vec3 diffuse = light.diffuse * diff * diffuseTex;
    vec3 specular = light.specular * spec * specularTex;

    float shadow =
        SpotShadowCalculation(
            fs_in.FragPosSpotLightSpace
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

vec3 getNormal()
{
    if (hasNormalMap)
    {
        vec3 normal = texture(texture_normal1, fs_in.TexCoords).rgb;
        normal = normal * 2.0 - 1.0;
        return normalize(fs_in.TBN * normal);
    }

    return normalize(fs_in.Normal);
}

void main()
{
    vec3 diffuseTex = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 specularTex = texture(texture_specular1, fs_in.TexCoords).rgb;
    vec3 normal = getNormal();
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);

    diffuseTex = pow(diffuseTex, vec3(2.2));
    specularTex = pow(specularTex, vec3(2.2));

    vec3 result = vec3(0.0);
    if (enablePointLight)
        result += calcPointLight(pointLight, 
                normal, 
                fs_in.FragPos, 
                viewDir, 
                diffuseTex, 
                specularTex);

    if (enableDirectionalLight)
        result += calcSun(sun, 
                normal, 
                viewDir, 
                diffuseTex, 
                specularTex);

    if (enableFlashlight)
        result += calcFlashLight(flashLight, 
                normal, 
                fs_in.FragPos, 
                viewDir, 
                diffuseTex, 
                specularTex);

    FragColor = vec4(result, 1.0);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > bloomThreshold)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

}
