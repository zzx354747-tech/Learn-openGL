#pragma once

class Framebuffer;
class PingPongFramebuffer;
class GBuffer;
class SSAO;
class Camera;
struct RendererScene;

struct WindowContext
{
    RendererScene* rendererScene = nullptr;

    // 输入状态
    bool cursorLocked        = true;
    bool gravePressLastFrame = false;
};
