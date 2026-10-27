#pragma once
#include <string>
#include <glad/gl.h>

class GLTexture
{
public:
    // Color maps use sRGB storage; normal/roughness/metallic maps remain linear.
    explicit GLTexture(const std::string& path, bool srgb = false);

    GLTexture(int width, int height,
          GLenum internalFormat,
          GLenum format,
          GLenum type,
          GLenum wrapMode,
          GLenum filterMode);

    ~GLTexture();

    // 禁止拷贝
    GLTexture(const GLTexture&)            = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    // 允许 move
    GLTexture(GLTexture&& other) noexcept;
    GLTexture& operator=(GLTexture&& other) noexcept;

    void bind(unsigned int unit) const;
    GLuint getID() const { return id; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    GLuint id = 0;
    int width = 0;
    int height = 0;
};
