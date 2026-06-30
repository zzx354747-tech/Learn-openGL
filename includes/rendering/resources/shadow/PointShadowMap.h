#pragma once

#include <glad/gl.h>
#include <iostream>

class PointShadowMap
{
public:
    PointShadowMap(int width, int height, float nearPlane, float farPlane);

    GLuint getDepthCubeMap() const;
    GLuint getFBO() const;
    float getNearPlane() const;
    float getFarPlane() const;
    int getWidth() const;
    int getHeight() const;

    ~PointShadowMap();

    PointShadowMap(const PointShadowMap&) = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

private:
    int width;
    int height;
    float nearPlane;
    float farPlane;
    GLuint fbo = 0;
    GLuint depthCubeMap = 0;

    void init();
};
