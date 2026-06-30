#pragma once

#include <glad/gl.h>
#include <iostream>

class SpotShadowMap
{
public:
    SpotShadowMap(int width, int height, float nearPlane, float farPlane);

    GLuint getDepthMap() const;
    GLuint getFBO() const;
    int getWidth() const;
    int getHeight() const;
    float getNearPlane() const;
    float getFarPlane() const;

    ~SpotShadowMap();

    SpotShadowMap(const SpotShadowMap&) = delete;
    SpotShadowMap& operator=(const SpotShadowMap&) = delete;

private:
    int width;
    int height;
    GLuint fbo = 0;
    GLuint depthMap = 0;
    float nearPlane;
    float farPlane;

    void init();
};
