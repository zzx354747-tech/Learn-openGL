#pragma once

class Framebuffer;
class PingPongFramebuffer;
class GBuffer;
class SSAO;
class Camera;

// 挂载在 GLFW window user pointer 上的上下文。
// 准入标准:只有 GLFW 回调需要读写的状态才放进来。
// 不拥有任何资源,所有指针指向 main 栈上的对象。
struct WindowContext
{
    // framebuffer_size_callback 的 resize 目标
    Framebuffer*         framebuffer = nullptr;
    PingPongFramebuffer* pingpongFBO = nullptr;
    GBuffer*             gBuffer     = nullptr;
    SSAO*                ssao        = nullptr;

    // mouse_callback / processInput 的输入目标
    Camera* camera = nullptr;

    // 输入状态
    bool cursorLocked        = true;
    bool gravePressLastFrame = false;
};