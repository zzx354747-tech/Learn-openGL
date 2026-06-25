#include "rendering/assets/HDRTexture.h"
#include "stb_image.h"
#include <algorithm>
#include <cmath>
#include "iostream"

namespace
{
constexpr float PI = 3.14159265359f;

float luminance(const glm::vec3& color)
{
    return glm::dot(color, glm::vec3(0.2126f, 0.7152f, 0.0722f));
}

glm::vec3 equirectangularDirection(int x, int y, int width, int height)
{
    float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
    float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
    float phi = u * 2.0f * PI - PI;
    float latitude = (v - 0.5f) * PI;
    float cosLatitude = std::cos(latitude);

    return glm::normalize(glm::vec3(
        cosLatitude * std::cos(phi),
        std::sin(latitude),
        cosLatitude * std::sin(phi)));
}

ExtractedLight extractSun(float* hdrData, int width, int height, int channels, const HDRLoadOptions& options)
{
    ExtractedLight sun;
    glm::vec3 weightedDir(0.0f);
    glm::vec3 totalRadiance(0.0f);
    float totalWeight = 0.0f;
    int pixelCount = 0;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = (y * width + x) * channels;
            glm::vec3 color(hdrData[idx], hdrData[idx + 1], hdrData[idx + 2]);
            float lum = luminance(color);

            if (lum > options.sunThreshold)
            {
                weightedDir += equirectangularDirection(x, y, width, height) * lum;
                totalRadiance += color;
                totalWeight += lum;
                ++pixelCount;
            }
        }
    }

    if (totalWeight <= 0.0f || pixelCount == 0)
    {
        return sun;
    }

    float pixelSolidAngle = (2.0f * PI / static_cast<float>(width))
        * (PI / static_cast<float>(height));
    sun.solidAngle = static_cast<float>(pixelCount) * pixelSolidAngle;
    sun.direction = glm::normalize(weightedDir / totalWeight);
    sun.color = (totalRadiance / static_cast<float>(pixelCount))
        * sun.solidAngle;
    sun.pixelCount = pixelCount;
    sun.valid = true;

    return sun;
}

void removeSun(float* hdrData, int width, int height, int channels, const HDRLoadOptions& options)
{
    for (int i = 0; i < width * height; ++i)
    {
        int idx = i * channels;
        glm::vec3 color(hdrData[idx], hdrData[idx + 1], hdrData[idx + 2]);
        float lum = luminance(color);

        if (lum > options.sunThreshold)
        {
            glm::vec3 chroma = lum > 0.0f ? color / lum : glm::vec3(1.0f);
            glm::vec3 replacement = chroma * options.sunReplaceValue;
            hdrData[idx] = replacement.r;
            hdrData[idx + 1] = replacement.g;
            hdrData[idx + 2] = replacement.b;
        }
    }
}
}

bool HDRTexture::load(const char* path, const HDRLoadOptions& options)
{
    extractedSun = ExtractedLight{};
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);
    stbi_set_flip_vertically_on_load(false);

    if (!data)
    {
        std::cerr << "Failed to load HDR image: " << path << std::endl;
        return false;
    }

    if (options.extractSun && nrComponents >= 3)
    {
        extractedSun = extractSun(data, width, height, nrComponents, options);
        if (extractedSun.valid)
        {
            removeSun(data, width, height, nrComponents, options);
        }
    }

    GLenum internalFormat = (nrComponents == 3) ? GL_RGB16F : GL_RGBA16F;
    GLenum dataFormat = (nrComponents == 3) ? GL_RGB : GL_RGBA;
    
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexImage2D(GL_TEXTURE_2D, 0, 
        internalFormat, width, 
        height, 0, 
        dataFormat, GL_FLOAT,
        data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return true;
}
