#pragma once
#include <string>
#include <glad/gl.h>

class GLTexture
{
public:
    explicit GLTexture(const std::string& path);

    ~GLTexture();

    // 禁止拷贝
    GLTexture(const GLTexture&)            = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    // 允许 move
    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

    void bind(unsigned int unit) const;
    GLuint getID() const { return id; }

private:
    GLuint id = 0;
};