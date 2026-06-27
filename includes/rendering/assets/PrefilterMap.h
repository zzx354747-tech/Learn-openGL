#pragma once
#include <glad/gl.h>
#include "rendering/assets/EnvCubemap.h"
#include "core/Shader.h"

class PrefilterMap {
public:
    PrefilterMap(const EnvCubemap& envCubemap, Shader& shader);
    ~PrefilterMap();

    PrefilterMap(const PrefilterMap&) = delete;
    PrefilterMap& operator=(const PrefilterMap&) = delete;

    unsigned int GetID() const { return m_prefilterID; }
    bool isReady() const { return m_prefilterID != 0; }

private:
    void Prefilter(unsigned int envCubemapID, Shader& shader);
    void RenderCube();

    unsigned int m_prefilterID = 0;
    unsigned int m_captureFBO  = 0;
    unsigned int m_cubeVAO     = 0;
    unsigned int m_cubeVBO     = 0;
    unsigned int m_cubeEBO     = 0;

    static constexpr unsigned int BASE_SIZE  = 128;
    static constexpr unsigned int MIP_LEVELS = 5;
};