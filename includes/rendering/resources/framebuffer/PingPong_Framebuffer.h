#pragma once 

#include <glad/gl.h>
#include <iostream>

class PingPongFramebuffer
{
public:
    PingPongFramebuffer(int width, int height);

    void resize(int width, int height);

    void bind(int index);

    void unbind();

    GLuint getTextureID(int index) const;

    ~PingPongFramebuffer();

private:
    GLuint fbos[2] = { 0, 0 };
    GLuint colorBuffers[2] = { 0, 0 };

    void initFramebuffer(int width, int height);
};
