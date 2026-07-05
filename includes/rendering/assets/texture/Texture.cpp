#include "rendering/assets/texture/Texture.h"
#include <stdexcept>

#include "stb_image.h"

GLTexture::GLTexture(const std::string& path)
{
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data)
    {
        glDeleteTextures(1, &id);
        id = 0;
        throw std::runtime_error("GLTexture: failed to load " + path);
    }

    GLenum format = GL_RGB;
    if      (channels == 1) format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;

    glTexImage2D(GL_TEXTURE_2D, 0, format,
                 width, height, 0,
                 format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
}

GLTexture::~GLTexture()
{
    if (id != 0)
        glDeleteTextures(1, &id);
}

GLTexture::GLTexture(GLTexture&& other) noexcept
    : id(other.id)
{
    other.id = 0;   // 转移所有权:other 不再负责释放
}

GLTexture& GLTexture::operator=(GLTexture&& other) noexcept
{
    if (this != &other)
    {
        if (id != 0)
            glDeleteTextures(1, &id);   // 释放自己原来持有的
        id       = other.id;
        other.id = 0;                   // 转移所有权
    }
    return *this;
}

void GLTexture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}
