#pragma once
#include <glad/gl.h>
#include <iostream>

class Framebuffer
{
public:
    Framebuffer(int width, int height);

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void resize(int width, int height);

    void bind();

    void unbind();

    unsigned int getTextureID(int index) const;

    unsigned int getFBO() const;

    ~Framebuffer();


    private:
        GLuint colorBuffers[2] = { 0, 0 };
        GLuint rboDepth = 0;
        GLuint hdrFBO = 0;

    void initFramebuffer(int bfwidth, int bfheight);
};
