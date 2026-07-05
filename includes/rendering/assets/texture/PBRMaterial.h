#pragma once
#include "rendering/assets/texture/Texture.h"
#include "core/Shader.h"
#include <glm/glm.hpp>
#include <optional>
#include <string>

class PBRMaterial
{
public:
    static PBRMaterial loadFromDirectory(const std::string& directoryPath);
    void bind(Shader& shader) const;

    std::optional<GLTexture> albedoTex;
    std::optional<GLTexture> normalTex;
    std::optional<GLTexture> metallicTex;
    std::optional<GLTexture> roughnessTex;
    std::optional<GLTexture> aoTex;
    std::optional<GLTexture> heightTex;

    glm::vec3 albedoValue   { 1.0f, 1.0f, 1.0f };
    float     metallicValue  = 0.0f;
    float     roughnessValue = 0.5f;
    float     heightScale    = 0.1f;
};
