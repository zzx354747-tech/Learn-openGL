#pragma once
#include <glad/gl.h>
#include <iostream>

class DirectionalShadowMap
{
public:
    DirectionalShadowMap( int width, int height);

    unsigned int getDepthMapTexture() const;

    unsigned int getFBO() const;
    int getWidth() const;
    int getHeight() const;

    DirectionalShadowMap(const DirectionalShadowMap&) = delete;
    DirectionalShadowMap& operator=(const DirectionalShadowMap&) = delete;
    ~DirectionalShadowMap();

private:
    int width;
    int height;
    unsigned int depthMapFBO = 0;
    unsigned int depthMap = 0;

    void initShadowMap();

};
