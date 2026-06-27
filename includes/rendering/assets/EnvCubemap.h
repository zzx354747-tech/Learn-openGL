#pragma once
#include "glad/gl.h"
#include "core/Shader.h"
#include "rendering/assets/HDRTexture.h"

class EnvCubemap 
{
public:
    EnvCubemap(HDRTexture& hdrTexture, Shader& shader);
    ~EnvCubemap();

    EnvCubemap(const EnvCubemap&) = delete;
    EnvCubemap& operator=(const EnvCubemap&) = delete;

    unsigned int getCubeMapID() const { return cubeMapID; }
    bool isReady() const { return cubeMapID != 0; }
    void bind(unsigned int unit = 0) const;
    void unbind() const;

private:
    HDRTexture& hdrTexture;
    GLuint cubeMapID = 0;
    GLuint cubeMapFBO = 0;
    GLuint cubeVAO = 0;
    GLuint cubeVBO = 0;

    void convert(Shader& shader);
    void RenderCube();

    unsigned int SIZE = 512;    
};
