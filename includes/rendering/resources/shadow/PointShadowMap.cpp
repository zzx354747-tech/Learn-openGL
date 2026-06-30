#include "rendering/resources/shadow/PointShadowMap.h"

PointShadowMap::PointShadowMap(int width, int height, float nearPlane, float farPlane) : width(width), height(height), nearPlane(nearPlane), farPlane(farPlane)
{
        init();
    }

GLuint PointShadowMap::getDepthCubeMap() const
{ return depthCubeMap; }

GLuint PointShadowMap::getFBO() const
{ return fbo; }

float PointShadowMap::getNearPlane() const
{ return nearPlane; }

float PointShadowMap::getFarPlane() const
{ return farPlane; }

int PointShadowMap::getWidth() const
{ return width; }

int PointShadowMap::getHeight() const
{ return height; }

PointShadowMap::~PointShadowMap()
{
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &depthCubeMap);
    }

void PointShadowMap::init()
{
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &depthCubeMap);

        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
        for (unsigned int i = 0; i < 6; i++)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                0, 
                GL_DEPTH_COMPONENT, 
                width, 
                height, 
                0, 
                GL_DEPTH_COMPONENT, 
                GL_FLOAT, 
                // 只申请显存，不上传数据
                nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // 绑定帧缓冲对象，并将深度纹理附加到它上面
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
        // 关闭颜色缓冲
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);

        // 检查帧缓冲对象是否完整
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Error: PointShadowMap framebuffer is not complete!" << std::endl;
        }

        // 解绑帧缓冲对象,解绑纹理
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
