#pragma once

#include "rendering/assets/texture/Texture.h"
#include "core/Shader.h"
#include "rendering/assets/mesh/Screenquad.h"
#include <glad/gl.h>

class BrightPrefilterPass {
public:
    BrightPrefilterPass(int width, int height, Shader& shader, Screenquad& quad);
    ~BrightPrefilterPass();

    void render(GLuint inputTexture);
    GLuint GetBrightPrefilterTextureID() const { return brightPrefilterTexture.getID(); }
    void resize(int width, int height);
    void setThreshold(float value) { threshold = value; }
    void setKnee(float value) { knee = value; }

private:
    Shader& brightPrefilterShader;
    Screenquad& screenQuad;
    GLuint BrightPrefilterFBO;
    GLTexture brightPrefilterTexture;
    float threshold = 1.0f;
    float knee = 0.5f;

    void initResources(int width, int height);  // 只负责 FBO 创建+附加,不再创建纹理
};
