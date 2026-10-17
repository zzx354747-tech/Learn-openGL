#pragma once 

#include "scene/sceneObject.h"

#include <glad/gl.h>
#include <iostream>
#include <vector>

// 该pass不输出任何画面，只输出一张全是ID的2D纹理

class ObjectPickPass {
public:
    ObjectPickPass(int width, int height);

    ObjectPickPass(const ObjectPickPass&) = delete;
    ObjectPickPass& operator=(const ObjectPickPass&) = delete;

    ~ObjectPickPass();

    void resize(int width, int height);
    void begin();
    void end();

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    SceneObjectID readID(int pixelX, int pixelY);

    private:
    unsigned int pickFBO = 0;
    unsigned int pickRBO = 0;
    unsigned int pickTexture = 0;
    int width = 0;
    int height = 0;

    struct SavedState
    {
        GLint drawFBO = 0;
        GLint readFBO = 0;
        GLint viewport[4] = {};
        GLint program = 0;
        GLint vertexArray = 0;
        GLint activeTexture = GL_TEXTURE0;
        std::vector<GLint> textures2D;
        GLboolean depthTest = GL_FALSE;
        GLboolean depthWrite = GL_TRUE;
        GLint depthFunc = GL_LESS;
        GLboolean blend = GL_FALSE;
        GLboolean stencilTest = GL_FALSE;
        GLboolean scissorTest = GL_FALSE;
        GLboolean colorMask[4] = {};
        GLint polygonMode[2] = {};
    } savedState;

    bool active = false;

    void initResources();
};
