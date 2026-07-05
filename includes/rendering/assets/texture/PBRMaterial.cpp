#include "rendering/assets/texture/PBRMaterial.h"
#include <filesystem>
#include <stdexcept>
#include <algorithm>

namespace fs = std::filesystem;

PBRMaterial PBRMaterial::loadFromDirectory(const std::string& directoryPath)
{
    PBRMaterial mat;
    static const std::vector<std::string> validExtensions = { ".png", ".jpg", ".jpeg", ".tga" };

    for (const auto& entry : fs::directory_iterator(directoryPath))
    {
        if (!entry.is_regular_file())
            continue;

        std::string ext = entry.path().extension().string();
        if (std::find(validExtensions.begin(), validExtensions.end(), ext) == validExtensions.end())
            continue;

        std::string filename = entry.path().filename().string();
        std::string fullPath = entry.path().string();

        auto assignOnce = [&](std::optional<GLTexture>& slot, const char* keyword)
        {
            if (filename.find(keyword) == std::string::npos)
                return false;
            if (slot.has_value())
            {
                throw std::runtime_error(
                    "PBRMaterial::loadFromDirectory: duplicate match for '" +
                    std::string(keyword) + "' in " + directoryPath +
                    " (offending file: " + filename + ")"
                );
            }
            slot.emplace(fullPath);
            return true;
        };

        if (assignOnce(mat.albedoTex,    "_albedo"))    continue;
        if (assignOnce(mat.normalTex,    "_normal"))    continue;
        if (assignOnce(mat.metallicTex,  "_metallic"))  continue;
        if (assignOnce(mat.roughnessTex, "_roughness")) continue;
        if (assignOnce(mat.aoTex,        "_ao"))        continue;
        if (assignOnce(mat.heightTex,    "_height"))    continue;
    }

    return mat;
}

void PBRMaterial::bind(Shader& shader) const
{
    // albedo
    bool hasAlbedo = albedoTex.has_value();
    shader.setBool("hasAlbedoMap", hasAlbedo);
    if (hasAlbedo)
    {
        albedoTex->bind(0);
        shader.setInt("albedoTexture", 0);
    }
    else
    {
        shader.setVec3("albedoColor", albedoValue);
    }

    // normal
    bool hasNormal = normalTex.has_value();
    shader.setBool("hasNormalMap", hasNormal);
    if (hasNormal)
    {
        normalTex->bind(1);
        shader.setInt("normalTexture", 1);
    }

    // height(parallax)
    bool hasHeight = heightTex.has_value();
    shader.setBool("hasParallaxMap", hasHeight);
    if (hasHeight)
    {
        heightTex->bind(2);
        shader.setInt("parallaxTexture", 2);
    }

    // roughness
    bool hasRoughness = roughnessTex.has_value();
    shader.setBool("hasRoughnessMap", hasRoughness);
    shader.setBool("usePackedMetallicRoughness", false);
    if (hasRoughness)
    {
        roughnessTex->bind(3);
        shader.setInt("roughnessTexture", 3);
    }
    else
    {
        shader.setFloat("roughnessFactor", roughnessValue);
    }

    // metallic
    bool hasMetallic = metallicTex.has_value();
    shader.setBool("hasMetallicMap", hasMetallic);
    if (hasMetallic)
    {
        metallicTex->bind(4);
        shader.setInt("metallicTexture", 4);
    }
    else
    {
        shader.setFloat("metallicFactor", metallicValue);
    }

    // ao:暂不处理,GBuffer 改造后再接
}
