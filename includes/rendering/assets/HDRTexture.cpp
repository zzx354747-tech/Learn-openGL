#include "rendering/assets/HDRTexture.h"
#include "stb_image.h"
#include "iostream"

bool HDRTexture::load(const char* path)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);
    stbi_set_flip_vertically_on_load(false);

    if (!data)
    {
        std::cerr << "Failed to load HDR image: " << path << std::endl;
        return false;
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