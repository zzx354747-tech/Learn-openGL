#pragma once
#include <glad/gl.h>
#include "rendering/assets/texture/EnvCubemap.h"
#include "core/Shader.h"

class IrradianceMap {
public:
    IrradianceMap(const EnvCubemap& envCubemap, Shader& shader);
    ~IrradianceMap();

    IrradianceMap(const IrradianceMap&) = delete;
    IrradianceMap& operator=(const IrradianceMap&) = delete;

    unsigned int GetID() const;
    bool isReady() const;

private:
    void Convolve(unsigned int envCubemapID, Shader& shader);
    void RenderCube();

    unsigned int m_irradianceID = 0;
    unsigned int m_captureFBO   = 0;
    unsigned int m_cubeVAO      = 0;
    unsigned int m_cubeVBO      = 0;

    static constexpr unsigned int SIZE = 32;
};
