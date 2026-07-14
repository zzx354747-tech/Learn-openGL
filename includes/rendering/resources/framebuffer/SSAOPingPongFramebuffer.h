#pragma once

#include <glad/gl.h>
#include <iostream>

// Two single-channel render targets used by the separable SSAO bilateral blur.
class SSAOPingPongFramebuffer
{
public:
    SSAOPingPongFramebuffer(int width, int height);

    SSAOPingPongFramebuffer(const SSAOPingPongFramebuffer&) = delete;
    SSAOPingPongFramebuffer& operator=(const SSAOPingPongFramebuffer&) = delete;

    void resize(int width, int height);

    void bind(int index);
    void unbind();

    GLuint getTextureID(int index) const;

    ~SSAOPingPongFramebuffer();

private:
    GLuint fbos[2] = { 0, 0 };
    GLuint colorBuffers[2] = { 0, 0 };

    void initFramebuffer(int width, int height);
};
