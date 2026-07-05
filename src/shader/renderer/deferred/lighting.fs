#version 330 core

// 声明编号0和1，会用来在drawbuffer列表中查询对应颜色附件的地址
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;

struct PointLight {
    vec3 position;
    vec3 diffuse;
    float constant;
    float linear;
    float quadratic;
};

struct Sun {
    vec3 direction;
    vec3 diffuse;
};

struct FlashLight {
    vec3 position;
    vec3 direction;
    vec3 diffuse;
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
uniform sampler2D gNormalRoughness;
uniform sampler2D gAlbedoMetallic;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// 接收SSAO贴图
uniform sampler2D AO;

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
uniform bool enableSSAO;
uniform bool enablePBR;
uniform bool enableIBL;

uniform float ssaoStrength;
uniform vec3 fixedAmbientColor;
uniform float fixedAmbientStrength;
uniform vec3 iblAmbientTint;
uniform float iblAmbientStrength;
uniform float phongDiffuseStrength;
uniform float phongSpecularStrength;
uniform float phongIBLDiffuseStrength;
uniform float phongIBLSpecularStrength;
uniform float bloomThreshold;
uniform float pointShadowStrength;
uniform float sunShadowStrength;
uniform float flashShadowStrength;
uniform float directionalShadowLightSize;
uniform float directionalShadowBlockerSearchRadius;
uniform float directionalShadowMinFilterRadius;
uniform float directionalShadowMaxFilterRadius;

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

const int PCSS_SAMPLE_COUNT = 16;
const vec2 PCSS_DISK[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2( 0.94558609, -0.76890725),
    vec2(-0.09418410, -0.92938870),
    vec2( 0.34495938,  0.29387760),
    vec2(-0.91588581,  0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543,  0.27676845),
    vec2( 0.97484398,  0.75648379),
    vec2( 0.44323325, -0.97511554),
    vec2( 0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2( 0.79197514,  0.19090188),
    vec2(-0.24188840,  0.99706507),
    vec2(-0.81409955,  0.91437590),
    vec2( 0.19984126,  0.78641367),
    vec2( 0.14383161, -0.14100790)
);

float findAverageBlockerDepth(vec3 projCoords, float bias)
{
    float blockerDepth = 0.0;
    int blockerCount = 0;

    for (int i = 0; i < PCSS_SAMPLE_COUNT; ++i)
    {
        float sampleDepth = texture(
            shadowMap,
            projCoords.xy + PCSS_DISK[i] * directionalShadowBlockerSearchRadius
        ).r;

        if (sampleDepth < projCoords.z - bias)
        {
            blockerDepth += sampleDepth;
            ++blockerCount;
        }
    }

    if (blockerCount == 0)
        return -1.0;

    return blockerDepth / float(blockerCount);
}

float filterPCSSShadow(vec3 projCoords, float bias, float filterRadius)
{
    float shadow = 0.0;

    for (int i = 0; i < PCSS_SAMPLE_COUNT; ++i)
    {
        float pcfDepth = texture(
            shadowMap,
            projCoords.xy + PCSS_DISK[i] * filterRadius
        ).r;

        shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
    }

    return shadow / float(PCSS_SAMPLE_COUNT);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // 将片段位置从裁剪空间转换到[0,1]范围内，告诉去哪里采样阴影贴图
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

     // 处理视野外的片段，防止出现不正确的阴影
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = 0.005;
    float avgBlockerDepth = findAverageBlockerDepth(projCoords, bias);
    if (avgBlockerDepth < 0.0)
        return 0.0;

    float penumbraRatio = max((projCoords.z - avgBlockerDepth) / max(avgBlockerDepth, 0.0001), 0.0);
    float filterRadius = clamp(
        penumbraRatio * directionalShadowLightSize,
        directionalShadowMinFilterRadius,
        directionalShadowMaxFilterRadius
    );

    return filterPCSSShadow(projCoords, bias, filterRadius);
}

// BRDF函数
// NDF,GGX模型
const float PI = 3.14159265359;

float D_GGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float denom  = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

// F函数,Schlick近似
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 高度相关G2函数，代入了渲染方程化简
float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness)
{
    float a2   = roughness * roughness * roughness * roughness;
    float GGXV = NdotL * sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    float GGXL = NdotV * sqrt(a2 + (1.0 - a2) * NdotL * NdotL);
    return 0.5 / (GGXV + GGXL + 1e-5);
}

vec3 calcPointLightPhong(PointLight light, vec3 normal, vec3 fragPos, 
                          vec3 viewDir, vec3 albedo, float roughness, float metallic)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 H = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float shininess = mix(8.0, 96.0, pow(1.0 - roughness, 2.0));
    float spec = pow(max(dot(normal, H), 0.0), shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    vec3 specularColor = mix(vec3(0.04), albedo, metallic);
    vec3 diffuse  = light.diffuse * diff * albedo * (phongDiffuseStrength / PI);
    vec3 specular = light.diffuse * spec * specularColor * phongSpecularStrength;
    float shadow = PointShadowCalculation(fragPos) * pointShadowStrength;
    return (diffuse + specular) * attenuation * (1.0 - shadow);
}

vec3 calcSunPhong(Sun light, vec3 normal, vec3 viewDir, 
                   vec3 albedo, vec4 FragPosLightSpace, float roughness, float metallic)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 H = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float shininess = mix(8.0, 96.0, pow(1.0 - roughness, 2.0));
    float spec = pow(max(dot(normal, H), 0.0), shininess);
    vec3 specularColor = mix(vec3(0.04), albedo, metallic);
    vec3 diffuse  = light.diffuse * diff * albedo * (phongDiffuseStrength / PI);
    vec3 specular = light.diffuse * spec * specularColor * phongSpecularStrength;
    float shadow = ShadowCalculation(FragPosLightSpace) * sunShadowStrength;
    return (diffuse + specular) * (1.0 - shadow);
}

vec3 calcFlashLightPhong(FlashLight light, vec3 normal, vec3 fragPos, 
                          vec3 viewDir, vec3 albedo, vec4 FragPosSpotLightSpace,
                          float roughness, float metallic)
{
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 H = normalize(lightDir + viewDir);
    float diff = max(dot(normal, lightDir), 0.0);
    float shininess = mix(8.0, 96.0, pow(1.0 - roughness, 2.0));
    float spec = pow(max(dot(normal, H), 0.0), shininess);
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    vec3 specularColor = mix(vec3(0.04), albedo, metallic);
    vec3 diffuse  = light.diffuse * diff * albedo * (phongDiffuseStrength / PI);
    vec3 specular = light.diffuse * spec * specularColor * phongSpecularStrength;
    float shadow = SpotShadowCalculation(FragPosSpotLightSpace) * flashShadowStrength;
    return (diffuse + specular) * attenuation * intensity * (1.0 - shadow);
}

vec3 calcPointLight(PointLight light, 
        vec3 normal, 
        vec3 fragPos, 
        vec3 viewDir, 
        vec3 albedo,
        vec3 F0,
        float roughness,
        float metallic)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // 计算半程向量
    vec3 H = normalize(lightDir + viewDir);
    // 计算NdotL,NdotV
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    // 调用BRDF函数，计算D、F、G
    float D = D_GGX(normal, H, roughness);
    vec3 F = F_Schlick(max(dot(H, viewDir), 0.0), F0);
    float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);

    // 能量守恒
    vec3 KS = F;
    vec3 KD = vec3(1.0) - KS;
    KD *= 1.0 - metallic;

    // 镜面反射部分
    vec3 specular = D * F * Vis;

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (
        light.constant +
        light.linear * distance +
        light.quadratic * distance * distance
    );

    vec3 radiance = light.diffuse * attenuation;

    // 渲染方程：(漫反射 + 镜面反射) × 辐射亮度 × NdotL
    vec3 Lo = (KD * albedo / PI + specular) * radiance * NdotL;

    float shadow = PointShadowCalculation(fragPos);
    shadow *= pointShadowStrength;

    return Lo * (1.0 - shadow);
}

vec3 calcSun(Sun light, 
vec3 normal, 
vec3 viewDir, 
vec3 albedo,
vec4 FragPosLightSpace,
vec3 F0,
float roughness,
float metallic)
{
    vec3 lightDir = normalize(-light.direction);

    // 计算半程向量
    vec3 H = normalize(lightDir + viewDir);
    // 计算NdotL,NdotV
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    // 调用BRDF函数，计算D、F、G
    float D = D_GGX(normal, H, roughness);
    vec3 F = F_Schlick(max(dot(H, viewDir), 0.0), F0);
    float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);

    // 能量守恒
    vec3 KS = F;
    vec3 KD = vec3(1.0) - KS;
    KD *= 1.0 - metallic;

    // 镜面反射部分
    vec3 specular = D * F * Vis;

    vec3 radiance = light.diffuse;

    // 渲染方程：(漫反射 + 镜面反射) × 辐射亮度 × NdotL
    vec3 Lo = (KD * albedo / PI + specular) * radiance * NdotL;

    float shadow = ShadowCalculation(FragPosLightSpace);

    shadow *= sunShadowStrength; // 将阴影强度应用到阴影值上

    return Lo * (1.0 - shadow);
}

vec3 calcFlashLight(FlashLight light, 
vec3 normal, 
vec3 fragPos, 
vec3 viewDir, 
vec3 albedo,
vec4 FragPosSpotLightSpace,
vec3 F0,
float roughness,
float metallic)
{
    vec3 lightDir = normalize(light.position - fragPos);

     // 计算半程向量
    vec3 H = normalize(lightDir + viewDir);
    // 计算NdotL,NdotV
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    // 调用BRDF函数，计算D、F、G
    float D = D_GGX(normal, H, roughness);
    vec3 F = F_Schlick(max(dot(H, viewDir), 0.0), F0);
    float Vis = V_SmithGGXCorrelated(NdotV, NdotL, roughness);

    // 能量守恒
    vec3 KS = F;
    vec3 KD = vec3(1.0) - KS;
    KD *= 1.0 - metallic;

    // 镜面反射部分
    vec3 specular = D * F * Vis;

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 radiance = light.diffuse * attenuation * intensity;

     // 渲染方程：(漫反射 + 镜面反射) × 辐射亮度 × NdotL
    vec3 Lo = (KD * albedo / PI + specular) * radiance * NdotL;

    float shadow =
        SpotShadowCalculation(
            FragPosSpotLightSpace
        );

    shadow *= flashShadowStrength;

    return Lo * (1.0 - shadow);
}

vec3 calcIBLAmbient(vec3 normal, vec3 fragPos,
vec3 camPos, vec3 albedo, 
float metallic, float roughness, 
float ao)
{
vec3 N = normalize(normal);
vec3 V = normalize(camPos - fragPos);
vec3 R = reflect(-V, N);

float NdotV = max(dot(N, V), 0.0);

// 菲涅尔（用粗糙度修正，避免 grazing angle 过亮）
vec3 F0 = mix(vec3(0.04), albedo, metallic);
vec3 F  = F0 + (max(vec3(1.0 - roughness), F0) - F0)
             * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

// 漫反射：irradiance map 直接采样
vec3 kS = F;
vec3 kD = (1.0 - kS) * (1.0 - metallic);
vec3 irradiance = texture(irradianceMap, N).rgb;
vec3 diffuse    = kD * irradiance * albedo;

// 镜面反射：prefilter + BRDF LUT
const float MAX_REFLECTION_LOD = 4.0;
vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
vec2 brdf  = texture(brdfLUT, vec2(NdotV, roughness)).rg;
vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

vec3 ambient = (diffuse + specular) * ao * iblAmbientTint * iblAmbientStrength;

return ambient;
}

vec3 calcPhongIBLAmbient(vec3 normal, vec3 fragPos,
vec3 camPos, vec3 albedo,
float metallic, float roughness,
float ao)
{
    vec3 N = normalize(normal);
    vec3 V = normalize(camPos - fragPos);
    vec3 R = reflect(-V, N);
    float NdotV = max(dot(N, V), 0.0);

    vec3 specularColor = mix(vec3(0.04), albedo, metallic);
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * albedo * (phongIBLDiffuseStrength / PI);

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor
        * (specularColor * brdf.x + brdf.y)
        * phongIBLSpecularStrength;

    float phongIBLBaseStrength = max(iblAmbientStrength, 0.45);
    return (diffuse + specular) * ao * iblAmbientTint * phongIBLBaseStrength;
}

vec3 calcFixedAmbient(vec3 albedo, float ao)
{
    return fixedAmbientColor * fixedAmbientStrength * albedo * ao;
}

void main()
{
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = normalize(texture(gNormalRoughness, TexCoords).rgb);
    vec3 albedo = texture(gAlbedoMetallic, TexCoords).rgb;
    float metallic = texture(gAlbedoMetallic, TexCoords).a;
    float roughness = texture(gNormalRoughness, TexCoords).a;
    // 现实中几乎非金属材料反射率都在2%-5%,金属材质反射率为它本身的颜色
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float aoSample = clamp(texture(AO, TexCoords).r, 0.0, 1.0);
    float ao = enableSSAO ? pow(aoSample, ssaoStrength) : 1.0;

    vec4 FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    vec4 FragPosSpotLightSpace = spotLightSpaceMatrix * vec4(FragPos, 1.0);

    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    if (enableIBL)
    {
        if (enablePBR)
            result += calcIBLAmbient(Normal, FragPos, viewPos, albedo, metallic, roughness, ao);
        else
            result += calcPhongIBLAmbient(Normal, FragPos, viewPos, albedo, metallic, roughness, ao);
    }
    else
    {
        result += calcFixedAmbient(albedo, ao);
    }

    if (enablePointLight)
{
    if (enablePBR)
        result += calcPointLight(pointLight, Normal, 
                    FragPos, viewDir, 
                    albedo, F0, 
                    roughness, metallic);
    else
        result += calcPointLightPhong(pointLight, Normal, 
                    FragPos, viewDir, 
                    albedo, roughness, 
                    metallic);
}

if (enableDirectionalLight)
{
    if (enablePBR)
        result += calcSun(sun, Normal, 
                    viewDir, albedo, 
                    FragPosLightSpace, F0,
                    roughness, metallic);
    else
        result += calcSunPhong(sun, Normal,
                    viewDir, albedo,
                    FragPosLightSpace,
                    roughness, metallic);
}

if (enableFlashlight)
{
    if (enablePBR)
        result += calcFlashLight(flashLight, Normal, 
                    FragPos, viewDir, 
                    albedo, FragPosSpotLightSpace,
                    F0, roughness,
                    metallic);
    else
        result += calcFlashLightPhong(flashLight, Normal, 
                    FragPos, viewDir, 
                    albedo, FragPosSpotLightSpace,
                    roughness, metallic);
}
    FragColor = vec4(result, 1.0);

    // 按人眼对三个颜色的敏感度计算亮度，如果亮度超过阈值，就把它写入BrightColor，否则写入黑色
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    
    if (brightness > bloomThreshold)
        BrightColor = vec4(result, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
