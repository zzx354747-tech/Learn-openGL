#version 330 core
out vec2 FragColor;
in vec2 TexCoords;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;
    float phi      = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up    = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 right = normalize(cross(up, N));
    up         = cross(N, right);

    return normalize(right * H.x + up * H.y + N * H.z);
}

// Smith G1 GGX
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0; // IBL 用的 k，和直接光不同
    return NdotV / (NdotV * (1.0 - k) + k);
}

// Smith G2 = G1(NdotV) * G1(NdotL)
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness)
         * GeometrySchlickGGX(NdotL, roughness);
}

void main()
{
    // UV 直接对应 NdotV 和 roughness
    float NdotV    = TexCoords.x;
    float roughness = TexCoords.y;

    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV); // 切线空间视线
    vec3 N = vec3(0.0, 0.0, 1.0);

    float scale = 0.0;
    float bias  = 0.0;

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G     = GeometrySmith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc    = pow(1.0 - VdotH, 5.0);

            scale += (1.0 - Fc) * G_Vis;
            bias  += Fc * G_Vis;
        }
    }

    scale /= float(SAMPLE_COUNT);
    bias  /= float(SAMPLE_COUNT);

    FragColor = vec2(scale, bias);
}