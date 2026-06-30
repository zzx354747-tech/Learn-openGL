#include "rendering/assets/texture/Texture.h"

GLTexture::GLTexture(const std::string& path) : path(path)
{
        initTex();
        initData(path);
    }

GLTexture::GLTexture(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : path("<solid>")
{
        initTex();
        unsigned char pixel[] = {r, g, b, a};
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

GLTexture::~GLTexture()
{
        glDeleteTextures(1, &id);
    }

void GLTexture::bind(unsigned int unit) const
{
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }

void GLTexture::initTex()
{
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

void GLTexture::initData(const std::string& path)
{
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            GLenum format = GL_RGB;
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;

            // 对齐要求，确保每行数据的起始地址是4字节对齐的
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        else
        {
            std::cout << "Failed to load texture at path: " << path << std::endl;
        }
        stbi_image_free(data);
    }
