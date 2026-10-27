#pragma once

#include <glad/gl.h>
#include "rendering/assets/mesh/Screenquad.h"
#include "rendering/assets/texture/Texture.h"
#include "core/Shader.h"

class DownSamplePass
{
public:
    DownSamplePass(int width, int height, Shader& shader, Screenquad& quad);
    ~DownSamplePass();

    void render(GLuint inputTexture, float threshold = 0.0f, float knee = 0.5f,
                float filterRadius = 1.0f);
    void resize(int width, int height);
    GLuint GetDownSampleTextureID() const { return downSampleTexture.getID(); }
    GLuint GetDownSampleFBO() const { return downSampleFBO; }
    int GetDownSampleWidth() const { return downSampleTexture.getWidth(); }
    int GetDownSampleHeight() const { return downSampleTexture.getHeight(); }

private:
    Shader& downSampleShader;
    Screenquad& screenQuad;
    GLuint downSampleFBO;
    GLTexture downSampleTexture;

    void initResources();  // 只负责 FBO 创建+附加,不再创建纹理
};
