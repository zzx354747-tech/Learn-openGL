#pragma once

#include "rendering/assets/EnvCubemap.h"
#include "rendering/postprocess/PingPong_Framebuffer.h"

struct SceneRenderOutputResources
{
    EnvCubemap* skybox = nullptr;
    PingPongFramebuffer* pingpongFBO = nullptr;
};
