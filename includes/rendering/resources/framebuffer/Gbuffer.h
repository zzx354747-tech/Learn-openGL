#pragma once

#include <glad/gl.h>
#include <iostream>
#include <rendering/resources/framebuffer/HDR_Framebuffer.h>

class GBuffer
{
public:
    GBuffer(int width, int height);

    void resize(int width, int height);

    void bind();

    void unbind();

    GLuint getFBO() const;

    unsigned int getGbufferTextureID(int index) const;

    unsigned int getPositionTexture() const;
    unsigned int getNormalRoughnessTexture() const;
    unsigned int getAlbedoMetallicTexture() const;
    unsigned int getVelocityTexture() const;

    void blitDepthTo(Framebuffer& framebuffer, int width, int height);

    ~GBuffer();

    GBuffer(const GBuffer&) = delete;
    GBuffer& operator=(const GBuffer&) = delete;

private:
    GLuint gBuffer = 0;
    GLuint attachments[4] = {0, 0, 0, 0};
    GLuint gPosition = 0;
    GLuint gNormalRoughness = 0;
    GLuint gAlbedoMetallic = 0;
    GLuint gVelocity = 0;
    GLuint rboDepth = 0;

    void initGBuffer(int width, int height);
};
