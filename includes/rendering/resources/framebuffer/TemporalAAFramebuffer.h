#pragma once

#include <glad/gl.h>

class TemporalAAFramebuffer
{
public:
    TemporalAAFramebuffer(int width, int height);
    ~TemporalAAFramebuffer();

    TemporalAAFramebuffer(const TemporalAAFramebuffer&) = delete;
    TemporalAAFramebuffer& operator=(const TemporalAAFramebuffer&) = delete;

    void resize(int width, int height);
    void bindWrite(int index) const;
    GLuint getTextureID(int index) const;

private:
    GLuint fbos_[2] = {0, 0};
    GLuint textures_[2] = {0, 0};

    void init(int width, int height);
};
