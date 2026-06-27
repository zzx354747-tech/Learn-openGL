#pragma once
#include <glad/gl.h>
#include "rendering/assets/EnvCubemap.h"
#include "core/Shader.h"

class IrradianceMap {
public:
    IrradianceMap(const EnvCubemap& envCubemap, Shader& shader);
    ~IrradianceMap();

    IrradianceMap(const IrradianceMap&) = delete;
    IrradianceMap& operator=(const IrradianceMap&) = delete;

    unsigned int GetID() const { return m_irradianceID; }
    bool isReady() const { return m_irradianceID != 0; }

private:
    void Convolve(unsigned int envCubemapID, Shader& shader);
    void RenderCube();

    unsigned int m_irradianceID = 0;
    unsigned int m_captureFBO   = 0;
    unsigned int m_cubeVAO      = 0;
    unsigned int m_cubeVBO      = 0;

    static constexpr unsigned int SIZE = 32;
};
