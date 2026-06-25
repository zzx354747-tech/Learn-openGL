#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

struct ExtractedLight
{
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(0.0f);
    int pixelCount = 0;
    float solidAngle = 0.0f;
    bool valid = false;
};

struct HDRLoadOptions
{
    bool extractSun = false;
    float sunThreshold = 100.0f;
    float sunReplaceValue = 2.0f;
};

class HDRTexture
{
public:
    HDRTexture() = default;
    // 禁止拷贝
    HDRTexture(const HDRTexture&) = delete;
    HDRTexture& operator=(const HDRTexture&) = delete;

    bool load(const char* path, const HDRLoadOptions& options = HDRLoadOptions{});
    // m_id为0为false,其他数字true
    bool isLoaded() const { return m_id;}

    unsigned int getId() const { return m_id; }
    const ExtractedLight& getExtractedSun() const { return extractedSun; }

    ~HDRTexture()
    {
        if (m_id)
        {
            glDeleteTextures(1, &m_id);
        }
    }

    private:
    unsigned int m_id = 0;
    ExtractedLight extractedSun;

};
