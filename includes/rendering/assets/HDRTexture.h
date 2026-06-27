#pragma once
#include <glad/gl.h>

class HDRTexture
{
public:
    HDRTexture() = default;
    // 禁止拷贝
    HDRTexture(const HDRTexture&) = delete;
    HDRTexture& operator=(const HDRTexture&) = delete;

    bool load(const char* path);
    // m_id为0为false,其他数字true
    bool isLoaded() const { return m_id;}

    unsigned int getId() const { return m_id; }

    ~HDRTexture()
    {
        if (m_id)
        {
            glDeleteTextures(1, &m_id);
        }
    }

    private:
    unsigned int m_id = 0;

};