#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

struct ExtractedLight
{
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 brightestDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    bool valid = false;
};

struct HDRLoadOptions
{
    bool      extractSun            = false;
    float     sunThreshold          = 100.0f;
};

class HDRTexture
{
public:
    HDRTexture() = default;
    HDRTexture(const HDRTexture&) = delete;
    HDRTexture& operator=(const HDRTexture&) = delete;

    bool load(const char* path, const HDRLoadOptions& options = HDRLoadOptions{});
    bool isLoaded() const;

    unsigned int getId() const;
    int getWidth() const;
    int getHeight() const;
    const ExtractedLight& getExtractedSun() const;

    ~HDRTexture();

private:
    unsigned int m_id = 0;
    int width         = 0;
    int height        = 0;
    ExtractedLight extractedSun;
};
