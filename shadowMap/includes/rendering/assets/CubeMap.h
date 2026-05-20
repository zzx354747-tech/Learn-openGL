#pragma once
#include <glad/gl.h>
#include <string>
#include <vector>
#include "stb_image.h"
#include <iostream>

class CubeMap
{
public:
    CubeMap(const std::vector<std::string>& faces)
    {
        initSkyboxTex();
        initSkyboxData(faces);
    }

    void bind(unsigned int unit = 0) const
    {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
    }

    void unbind() const
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    ~CubeMap()
    {
        glDeleteTextures(1, &id);
    }

    CubeMap(const CubeMap&) = delete;
    CubeMap& operator=(const CubeMap&) = delete;

private:
    unsigned int id = 0;
    int width = 0, height = 0, nrChannels = 0;

    void initSkyboxTex()
    {
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_CUBE_MAP, id);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    void initSkyboxData(const std::vector<std::string>& skyboxFaces)
    {

        for (unsigned int i = 0; i < skyboxFaces.size(); i++)
        {
            unsigned char* data = stbi_load(skyboxFaces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                GLenum format = GL_RGB;
                if (nrChannels == 1)
                    format = GL_RED;    
                else if (nrChannels == 3)
                    format = GL_RGB;
                else if (nrChannels == 4)
                    format = GL_RGBA;

                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Failed to load skybox texture at path: " << skyboxFaces[i] << std::endl;
                stbi_image_free(data);
            }
        }
    }
};
    
