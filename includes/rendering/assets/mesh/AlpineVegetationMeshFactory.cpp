#include "rendering/assets/mesh/AlpineVegetationMeshFactory.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

#include <assimp/GltfMaterial.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <stb_image.h>

namespace
{
constexpr float Pi = 3.14159265358979323846f;

std::filesystem::path vegetationAssetDirectory()
{
#ifdef OPENGL_PROJECT_ROOT
    return std::filesystem::path(OPENGL_PROJECT_ROOT) /
           "resources" / "models" / "vegetation";
#else
    return std::filesystem::path("../resources/models/vegetation");
#endif
}

std::filesystem::path bakedVegetationAssetDirectory()
{
#ifdef OPENGL_PROJECT_ROOT
    return std::filesystem::path(OPENGL_PROJECT_ROOT) /
           "resources" / "source_models" / "botaniq_baked";
#else
    return std::filesystem::path("../resources/source_models/botaniq_baked");
#endif
}

std::string lowercase(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::optional<std::string> readTextFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return std::nullopt;
    std::ostringstream contents;
    contents << stream.rdbuf();
    return contents.str();
}

std::size_t jsonValuePosition(const std::string& text, const char* key,
                              std::size_t begin, std::size_t end)
{
    const std::string token = std::string("\"") + key + "\"";
    const std::size_t keyPosition = text.find(token, begin);
    if (keyPosition == std::string::npos || keyPosition >= end)
        return std::string::npos;
    const std::size_t colon = text.find(':', keyPosition + token.size());
    if (colon == std::string::npos || colon >= end)
        return std::string::npos;
    return text.find_first_not_of(" \t\r\n", colon + 1);
}

std::optional<std::string> jsonString(const std::string& text, const char* key,
                                      std::size_t begin, std::size_t end)
{
    std::size_t position = jsonValuePosition(text, key, begin, end);
    if (position == std::string::npos || position >= end ||
        text[position] != '"')
        return std::nullopt;
    ++position;
    std::string value;
    bool escaped = false;
    for (; position < end; ++position)
    {
        const char c = text[position];
        if (escaped)
        {
            value.push_back(c);
            escaped = false;
        }
        else if (c == '\\')
            escaped = true;
        else if (c == '"')
            return value;
        else
            value.push_back(c);
    }
    return std::nullopt;
}

std::optional<float> jsonFloat(const std::string& text, const char* key,
                               std::size_t begin, std::size_t end)
{
    const std::size_t position = jsonValuePosition(text, key, begin, end);
    if (position == std::string::npos || position >= end)
        return std::nullopt;
    char* parsedEnd = nullptr;
    const float value = std::strtof(text.c_str() + position, &parsedEnd);
    if (parsedEnd == text.c_str() + position)
        return std::nullopt;
    return value;
}

bool jsonBool(const std::string& text, const char* key, std::size_t begin,
              std::size_t end, bool fallback)
{
    const std::size_t position = jsonValuePosition(text, key, begin, end);
    if (position == std::string::npos || position >= end)
        return fallback;
    if (text.compare(position, 4, "true") == 0)
        return true;
    if (text.compare(position, 5, "false") == 0)
        return false;
    return fallback;
}

std::optional<glm::vec4> jsonVec4(const std::string& text, const char* key,
                                  std::size_t begin, std::size_t end)
{
    std::size_t position = jsonValuePosition(text, key, begin, end);
    if (position == std::string::npos || position >= end ||
        text[position] != '[')
        return std::nullopt;
    ++position;
    glm::vec4 value(0.0f);
    for (int component = 0; component < 4; ++component)
    {
        position = text.find_first_not_of(" \t\r\n,", position);
        if (position == std::string::npos || position >= end)
            return std::nullopt;
        char* parsedEnd = nullptr;
        value[component] = std::strtof(text.c_str() + position, &parsedEnd);
        if (parsedEnd == text.c_str() + position)
            return std::nullopt;
        position = static_cast<std::size_t>(parsedEnd - text.c_str());
    }
    return value;
}

std::vector<std::pair<std::size_t, std::size_t>> jsonArrayObjects(
    const std::string& text, const char* key)
{
    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    const std::size_t value = jsonValuePosition(text, key, 0, text.size());
    if (value == std::string::npos || text[value] != '[')
        return ranges;

    int braceDepth = 0;
    bool inString = false;
    bool escaped = false;
    std::size_t objectStart = std::string::npos;
    for (std::size_t i = value + 1; i < text.size(); ++i)
    {
        const char c = text[i];
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (c == '\\')
                escaped = true;
            else if (c == '"')
                inString = false;
            continue;
        }
        if (c == '"')
        {
            inString = true;
            continue;
        }
        if (c == '{')
        {
            if (braceDepth == 0)
                objectStart = i;
            ++braceDepth;
        }
        else if (c == '}')
        {
            --braceDepth;
            if (braceDepth == 0 && objectStart != std::string::npos)
            {
                ranges.emplace_back(objectStart, i + 1);
                objectStart = std::string::npos;
            }
        }
        else if (c == ']' && braceDepth == 0)
            break;
    }
    return ranges;
}

bool decodeImageFile(const std::filesystem::path& path,
                     VegetationImageData& output)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* decoded = stbi_load(
        path.string().c_str(), &width, &height, &channels, 4);
    if (!decoded || width <= 0 || height <= 0)
    {
        if (decoded)
            stbi_image_free(decoded);
        return false;
    }
    output.width = width;
    output.height = height;
    output.rgba.assign(decoded, decoded +
        static_cast<std::size_t>(width) * height * 4u);
    stbi_image_free(decoded);
    // glTF TEXCOORD_0 uses a top-left image origin. Uploading the PNG rows in
    // their file order already converts that convention to OpenGL's bottom-left
    // texture storage. Flipping here a second time makes opacity masks sample
    // upside-down and visually cuts cards into disconnected fragments.
    return true;
}

struct RuntimeMaterialDescriptor
{
    std::array<std::string, 4> materialNames{};
    std::array<VegetationMaterialSlot, 4> slots{};
    int materialCount = 0;
    std::vector<VegetationImageData> baseColorMips;
    std::vector<VegetationImageData> normalMips;
    std::vector<VegetationImageData> foliageDataMips;

    bool valid() const
    {
        return materialCount > 0;
    }

    bool hasImages() const
    {
        return !baseColorMips.empty() && !normalMips.empty() &&
               !foliageDataMips.empty();
    }
};

std::vector<VegetationImageData> loadAtlasMipChain(
    const std::filesystem::path& atlasDirectory, const char* prefix)
{
    std::vector<VegetationImageData> mips;
    for (int level = 0; level < 16; ++level)
    {
        const std::filesystem::path path =
            atlasDirectory / (std::string(prefix) + std::to_string(level) + ".png");
        if (!std::filesystem::exists(path))
            break;
        VegetationImageData image;
        if (!decodeImageFile(path, image))
        {
            std::cerr << "Vegetation atlas mip decode failed: "
                      << path.string() << std::endl;
            mips.clear();
            break;
        }
        if (!mips.empty())
        {
            const int expectedWidth = std::max(1, mips.back().width / 2);
            const int expectedHeight = std::max(1, mips.back().height / 2);
            if (image.width != expectedWidth || image.height != expectedHeight)
            {
                std::cerr << "Vegetation atlas mip dimensions are invalid: "
                          << path.string() << std::endl;
                mips.clear();
                break;
            }
        }
        mips.push_back(std::move(image));
    }
    return mips;
}

RuntimeMaterialDescriptor loadRuntimeMaterialDescriptor(
    const std::string& assetName, bool loadImages)
{
    RuntimeMaterialDescriptor descriptor;
    const std::filesystem::path assetDirectory =
        bakedVegetationAssetDirectory() / assetName;
    const std::filesystem::path descriptorPath =
        assetDirectory / "material.json";
    const std::optional<std::string> text = readTextFile(descriptorPath);
    if (!text)
    {
        std::cerr << "Vegetation material descriptor missing: "
                  << descriptorPath.string() << std::endl;
        return descriptor;
    }
    if (text->find("\"schema\": \"openai.botaniq-runtime-material/1\"") ==
        std::string::npos)
    {
        std::cerr << "Vegetation material descriptor schema unsupported: "
                  << descriptorPath.string() << std::endl;
        return descriptor;
    }

    const auto objects = jsonArrayObjects(*text, "materials");
    descriptor.materialCount = static_cast<int>(
        std::min<std::size_t>(objects.size(), descriptor.slots.size()));
    for (int i = 0; i < descriptor.materialCount; ++i)
    {
        const auto [begin, end] = objects[static_cast<std::size_t>(i)];
        descriptor.materialNames[static_cast<std::size_t>(i)] =
            lowercase(jsonString(*text, "name", begin, end).value_or(""));
        descriptor.slots[static_cast<std::size_t>(i)].tileRect =
            jsonVec4(*text, "tile_rect", begin, end).value_or(
                glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
        // The descriptor was authored for a vertically flipped OpenGL upload.
        // Atlas PNGs are now uploaded in glTF row order, so mirror only the
        // tile's atlas row; local TEXCOORD orientation remains untouched.
        VegetationMaterialSlot& slot =
            descriptor.slots[static_cast<std::size_t>(i)];
        slot.tileRect.y = 1.0f - slot.tileRect.y - slot.tileRect.w;
        const std::string alphaMode =
            jsonString(*text, "alpha_mode", begin, end).value_or("OPAQUE");
        slot.alphaMask = alphaMode == "MASK";
        slot.alphaCutoff =
            jsonFloat(*text, "alpha_cutoff", begin, end).value_or(0.5f);
        slot.doubleSided =
            jsonBool(*text, "flexible", begin, end, false);
    }

    if (loadImages && descriptor.valid())
    {
        const std::filesystem::path atlasDirectory = assetDirectory / "atlas";
        descriptor.baseColorMips =
            loadAtlasMipChain(atlasDirectory, "basecolor_mip");
        descriptor.normalMips =
            loadAtlasMipChain(atlasDirectory, "normal_mip");
        descriptor.foliageDataMips =
            loadAtlasMipChain(atlasDirectory, "foliage_data_mip");
        if (!descriptor.hasImages() ||
            descriptor.baseColorMips.size() != descriptor.normalMips.size() ||
            descriptor.baseColorMips.size() != descriptor.foliageDataMips.size())
        {
            std::cerr << "Vegetation atlas mip chains are incomplete: "
                      << assetDirectory.string() << std::endl;
            descriptor.baseColorMips.clear();
            descriptor.normalMips.clear();
            descriptor.foliageDataMips.clear();
        }
        else
        {
            std::cout << "Vegetation runtime material loaded: " << assetName
                      << " (" << descriptor.materialCount << " materials, "
                      << descriptor.baseColorMips.size()
                      << " explicit mips)" << std::endl;
        }
    }
    return descriptor;
}

int materialSlotIndex(const RuntimeMaterialDescriptor& descriptor,
                      const std::string& materialName)
{
    const std::string name = lowercase(materialName);
    for (int i = 0; i < descriptor.materialCount; ++i)
    {
        const std::string& candidate =
            descriptor.materialNames[static_cast<std::size_t>(i)];
        if (name == candidate ||
            (name.size() > candidate.size() &&
             name.compare(0, candidate.size(), candidate) == 0 &&
             name[candidate.size()] == '.'))
            return i;
    }
    return -1;
}

bool decodeTexture(const aiScene* scene, const aiMaterial* material,
                   aiTextureType type, const std::filesystem::path& directory,
                   VegetationImageData& output, std::string& key)
{
    if (!material || material->GetTextureCount(type) == 0)
        return false;

    aiString texturePath;
    if (material->GetTexture(type, 0, &texturePath) != AI_SUCCESS)
        return false;
    key = texturePath.C_Str();

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* decoded = nullptr;
    const aiTexture* embedded = scene->GetEmbeddedTexture(texturePath.C_Str());
    if (embedded)
    {
        if (embedded->mHeight == 0)
        {
            decoded = stbi_load_from_memory(
                reinterpret_cast<const unsigned char*>(embedded->pcData),
                static_cast<int>(embedded->mWidth), &width, &height, &channels, 4);
        }
        else
        {
            width = static_cast<int>(embedded->mWidth);
            height = static_cast<int>(embedded->mHeight);
            decoded = static_cast<unsigned char*>(std::malloc(
                static_cast<std::size_t>(width) * height * 4u));
            if (decoded)
            {
                for (int i = 0; i < width * height; ++i)
                {
                    decoded[i * 4 + 0] = embedded->pcData[i].r;
                    decoded[i * 4 + 1] = embedded->pcData[i].g;
                    decoded[i * 4 + 2] = embedded->pcData[i].b;
                    decoded[i * 4 + 3] = embedded->pcData[i].a;
                }
            }
        }
    }
    else
    {
        const std::filesystem::path resolved = directory / texturePath.C_Str();
        key = resolved.lexically_normal().string();
        decoded = stbi_load(resolved.string().c_str(), &width, &height, &channels, 4);
    }

    if (!decoded || width <= 0 || height <= 0)
    {
        if (decoded) stbi_image_free(decoded);
        return false;
    }

    output.width = width;
    output.height = height;
    output.rgba.assign(decoded, decoded +
        static_cast<std::size_t>(width) * height * 4u);
    stbi_image_free(decoded);
    // Keep the embedded glTF image row convention aligned with TEXCOORD_0.
    return true;
}

std::optional<VegetationMeshData> loadVegetationAsset(
    const std::filesystem::path& path, const std::string& assetName,
    float targetHeight, float windStrength, float shapeVariation,
    bool loadRuntimeImages)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path.string(),
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices |
        aiProcess_SortByPType);
    if (!scene || !scene->mRootNode)
    {
        std::cerr << "Vegetation asset load failed: " << path.string()
                  << " (" << importer.GetErrorString() << ")" << std::endl;
        return std::nullopt;
    }

    VegetationMeshData result;
    RuntimeMaterialDescriptor runtimeMaterial =
        loadRuntimeMaterialDescriptor(assetName, loadRuntimeImages);
    if (runtimeMaterial.valid())
    {
        result.materialCount = runtimeMaterial.materialCount;
        result.materialSlots = runtimeMaterial.slots;
        result.baseColorAtlasMips = std::move(runtimeMaterial.baseColorMips);
        result.normalAtlasMips = std::move(runtimeMaterial.normalMips);
        result.foliageDataAtlasMips = std::move(runtimeMaterial.foliageDataMips);
    }
    std::vector<float> windMultipliers;
    std::vector<std::string> selectedBaseColorKeys;
    std::vector<std::string> selectedNormalKeys;
    std::array<bool, 4> assignedRuntimeSlots{};
    std::vector<int> runtimeSlotByAssimpMaterial(
        scene->mNumMaterials, -1);
    const auto storeTexture = [](const std::string& textureKey,
                                 VegetationImageData image,
                                 std::vector<std::string>& keys,
                                 std::vector<VegetationImageData>& textures)
    {
        const auto found = std::find(keys.begin(), keys.end(), textureKey);
        if (found != keys.end())
            return static_cast<float>(std::distance(keys.begin(), found) + 1);
        if (keys.size() >= 4u)
            return 0.0f;
        keys.push_back(textureKey);
        textures.push_back(std::move(image));
        return static_cast<float>(keys.size());
    };

    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        if (!mesh || !(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
            continue;

        glm::vec4 baseColor(1.0f);
        float roughness = 0.88f;
        float materialWind = 1.0f;
        bool materialDoubleSided = false;
        bool materialAlpha = false;
        float alphaCutoff = 0.5f;
        float baseTextureWeight = 0.0f;
        float normalTextureWeight = 0.0f;

        const aiMaterial* material = mesh->mMaterialIndex < scene->mNumMaterials
            ? scene->mMaterials[mesh->mMaterialIndex] : nullptr;
        if (material)
        {
            aiString materialName;
            material->Get(AI_MATKEY_NAME, materialName);
            const std::string name = lowercase(materialName.C_Str());
            const bool flexible = name.find("leaf") != std::string::npos ||
                                  name.find("grass") != std::string::npos ||
                                  name.find("flower") != std::string::npos;
            // Every vertex at the same height must inherit the same primary
            // bend. Material-dependent translation detached leaf cards from
            // their supporting branches. Fine leaf flutter belongs in a
            // secondary deformation that preserves the attachment point.
            materialWind = 1.0f;

            aiColor4D color;
            if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
                baseColor = glm::vec4(color.r, color.g, color.b, color.a);
            else
            {
                aiColor3D diffuse;
                if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
                    baseColor = glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f);
            }
            material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
            material->Get(AI_MATKEY_TWOSIDED, materialDoubleSided);
            material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff);
            aiString alphaMode;
            if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
                materialAlpha = std::string(alphaMode.C_Str()) != "OPAQUE";
            materialDoubleSided = materialDoubleSided || flexible;

            int runtimeSlot = materialSlotIndex(runtimeMaterial, name);
            if (runtimeSlot >= 0)
                assignedRuntimeSlots[static_cast<std::size_t>(runtimeSlot)] = true;
            else if (runtimeMaterial.valid())
            {
                int& fallbackSlot =
                    runtimeSlotByAssimpMaterial[mesh->mMaterialIndex];
                if (fallbackSlot < 0)
                {
                    for (int candidate = 0;
                         candidate < runtimeMaterial.materialCount; ++candidate)
                    {
                        if (!assignedRuntimeSlots[
                                static_cast<std::size_t>(candidate)])
                        {
                            fallbackSlot = candidate;
                            assignedRuntimeSlots[
                                static_cast<std::size_t>(candidate)] = true;
                            break;
                        }
                    }
                }
                runtimeSlot = fallbackSlot;
                std::cerr << "Vegetation material name '" << name
                          << "' was not present in " << assetName
                          << "/material.json; using descriptor slot "
                          << runtimeSlot << std::endl;
            }
            if (runtimeSlot >= 0)
            {
                const VegetationMaterialSlot& slot =
                    runtimeMaterial.slots[static_cast<std::size_t>(runtimeSlot)];
                baseTextureWeight = static_cast<float>(runtimeSlot + 1);
                normalTextureWeight = baseTextureWeight;
                baseColor = glm::vec4(1.0f);
                materialAlpha = slot.alphaMask;
                materialDoubleSided = materialDoubleSided || slot.doubleSided;
                alphaCutoff = slot.alphaCutoff;
            }
            else
            {
                VegetationImageData image;
                std::string textureKey;
                bool hasBaseColor = decodeTexture(
                    scene, material, aiTextureType_BASE_COLOR, path.parent_path(),
                    image, textureKey);
                if (!hasBaseColor)
                    hasBaseColor = decodeTexture(
                        scene, material, aiTextureType_DIFFUSE, path.parent_path(),
                        image, textureKey);
                // Several botaniq GLB exports contain only a neutral 0.8 material
                // factor because linked-library node groups are not exportable.
                // Preserve authored textures when present; otherwise recover an
                // ecologically plausible material colour from the stable botaniq
                // material name instead of rendering the whole asset grey-white.
                if (!hasBaseColor)
                {
                    glm::vec3 fallbackColor(0.12f, 0.24f, 0.045f);
                    if (name.find("bark") != std::string::npos)
                        fallbackColor = glm::vec3(0.105f, 0.052f, 0.020f);
                    else if (name.find("leaf_picea") != std::string::npos)
                        fallbackColor = glm::vec3(0.025f, 0.105f, 0.024f);
                    else if (name.find("leaf_larix") != std::string::npos)
                        fallbackColor = glm::vec3(0.095f, 0.205f, 0.035f);
                    else if (name.find("leaf") != std::string::npos)
                        fallbackColor = glm::vec3(0.075f, 0.185f, 0.035f);
                    else if (name.find("stem") != std::string::npos)
                        fallbackColor = glm::vec3(0.095f, 0.175f, 0.030f);
                    else if (name.find("grass") != std::string::npos)
                        fallbackColor = glm::vec3(0.085f, 0.220f, 0.035f);
                    else if (name.find("achillea") != std::string::npos)
                        fallbackColor = name.find("flower") != std::string::npos
                            ? glm::vec3(0.78f, 0.74f, 0.58f)
                            : glm::vec3(0.075f, 0.180f, 0.030f);
                    else if (name.find("bellflower") != std::string::npos)
                        fallbackColor = glm::vec3(0.16f, 0.20f, 0.72f);
                    else if (name.find("dahlia") != std::string::npos)
                        fallbackColor = name.find("flower") != std::string::npos
                            ? glm::vec3(0.62f, 0.025f, 0.018f)
                            : glm::vec3(0.065f, 0.175f, 0.028f);
                    else if (name.find("flower_summer") != std::string::npos)
                        fallbackColor = glm::vec3(0.92f, 0.46f, 0.025f);
                    else if (name.find("snow") != std::string::npos)
                        fallbackColor = glm::vec3(0.68f, 0.72f, 0.76f);
                    baseColor = glm::vec4(fallbackColor, baseColor.a);
                }
                if (hasBaseColor)
                {
                    baseTextureWeight = storeTexture(
                        textureKey, std::move(image), selectedBaseColorKeys,
                        result.baseColorTextures);
                    materialAlpha = true;
                }

                VegetationImageData normalImage;
                std::string normalKey;
                if (decodeTexture(scene, material, aiTextureType_NORMALS,
                                  path.parent_path(), normalImage, normalKey))
                {
                    normalTextureWeight = storeTexture(
                        normalKey, std::move(normalImage), selectedNormalKeys,
                        result.normalTextures);
                }
            }

            result.alphaMask = result.alphaMask || materialAlpha;
            result.doubleSided = result.doubleSided || materialDoubleSided;
            result.alphaCutoff = std::min(result.alphaCutoff, alphaCutoff);
        }

        const std::uint32_t baseVertex =
            static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.reserve(result.vertices.size() + mesh->mNumVertices);
        windMultipliers.reserve(windMultipliers.size() + mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            const aiVector3D& p = mesh->mVertices[i];
            const aiVector3D n = mesh->HasNormals()
                ? mesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
            glm::vec2 uv(0.0f);
            if (mesh->HasTextureCoords(0))
                uv = glm::vec2(mesh->mTextureCoords[0][i].x,
                               mesh->mTextureCoords[0][i].y);
            result.vertices.push_back({
                glm::vec3(p.x, p.y, p.z),
                glm::normalize(glm::vec3(n.x, n.y, n.z)),
                glm::vec4(glm::vec3(baseColor), glm::clamp(roughness, 0.04f, 1.0f)),
                glm::vec2(0.0f, shapeVariation),
                glm::vec4(uv, baseTextureWeight, normalTextureWeight)});
            windMultipliers.push_back(materialWind);
        }
        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            const aiFace& face = mesh->mFaces[faceIndex];
            if (face.mNumIndices != 3) continue;
            result.indices.push_back(baseVertex + face.mIndices[0]);
            result.indices.push_back(baseVertex + face.mIndices[1]);
            result.indices.push_back(baseVertex + face.mIndices[2]);
        }
    }

    if (result.vertices.empty() || result.indices.empty())
        return std::nullopt;

    // Blender's glTF exporter and Assimp's glTF importer both use Y-up here.
    // Guessing the up axis from the largest bounding-box extent rotates wide
    // crowns and flower clusters sideways, and can invert already-correct
    // assets. The pre-transformed GLB positions must be used as authored.
    result.updateBounds();
    const float sourceHeight = std::max(result.boundsMax.y - result.boundsMin.y, 1e-4f);
    const float scale = targetHeight / sourceHeight;
    const glm::vec2 center(
        (result.boundsMin.x + result.boundsMax.x) * 0.5f,
        (result.boundsMin.z + result.boundsMax.z) * 0.5f);
    const float bottom = result.boundsMin.y;
    for (std::size_t i = 0; i < result.vertices.size(); ++i)
    {
        VegetationVertex& vertex = result.vertices[i];
        vertex.position = glm::vec3(
            (vertex.position.x - center.x) * scale,
            (vertex.position.y - bottom) * scale,
            (vertex.position.z - center.y) * scale);
        const float heightWeight = glm::clamp(vertex.position.y / targetHeight, 0.0f, 1.0f);
        vertex.windVariation.x = std::pow(heightWeight, 1.35f) *
                                 windStrength * windMultipliers[i];
    }
    result.updateBounds();
    result.externalAsset = true;
    return result;
}

struct Random
{
    std::uint32_t state;
    float unit()
    {
        state = state * 1664525u + 1013904223u;
        return static_cast<float>((state >> 8u) & 0x00ffffffu) / 16777215.0f;
    }
    float range(float a, float b) { return glm::mix(a, b, unit()); }
};

void vertex(VegetationMeshData& mesh, const glm::vec3& p,
            const glm::vec3& n, const glm::vec3& color, float roughness,
            float wind, float variation)
{
    mesh.vertices.push_back({p, n, glm::vec4(color, roughness),
                             glm::vec2(wind, variation)});
}

void triangle(VegetationMeshData& mesh, const glm::vec3& a,
              const glm::vec3& b, const glm::vec3& c,
              const glm::vec3& color, float roughness, float wind,
              float variation, float upwardBias = 0.0f)
{
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    if (n.y < -0.92f && upwardBias > 0.0f)
        n = glm::normalize(n + glm::vec3(0.0f, upwardBias * 0.35f, 0.0f));
    else if (upwardBias > 0.0f)
        n = glm::normalize(n * (1.0f - upwardBias) +
                           glm::vec3(0.0f, 1.0f, 0.0f) * upwardBias);
    const std::uint16_t first = static_cast<std::uint16_t>(mesh.vertices.size());
    vertex(mesh, a, n, color, roughness, wind, variation);
    vertex(mesh, b, n, color, roughness, wind, variation);
    vertex(mesh, c, n, color, roughness, wind, variation);
    mesh.indices.insert(mesh.indices.end(), {first, static_cast<std::uint16_t>(first + 1u),
                                             static_cast<std::uint16_t>(first + 2u)});
}

void quad(VegetationMeshData& mesh, const glm::vec3& a, const glm::vec3& b,
          const glm::vec3& c, const glm::vec3& d, const glm::vec3& color,
          float roughness, float wind, float variation, float upwardBias = 0.0f)
{
    triangle(mesh, a, b, c, color, roughness, wind, variation, upwardBias);
    triangle(mesh, a, c, d, color, roughness, wind, variation, upwardBias);
}

void appendTaperedPrism(VegetationMeshData& mesh, float height, int sides)
{
    constexpr std::array<float, 5> heights = {-0.05f, 0.0f, 0.40f, 0.75f, 0.98f};
    constexpr std::array<float, 5> radii = {0.045f, 0.036f, 0.025f, 0.014f, 0.004f};
    const glm::vec3 trunk(0.16f, 0.075f, 0.025f);
    for (int ring = 0; ring < 4; ++ring)
    {
        for (int i = 0; i < sides; ++i)
        {
            const float a0 = 2.0f * Pi * static_cast<float>(i) / sides;
            const float a1 = 2.0f * Pi * static_cast<float>(i + 1) / sides;
            const glm::vec3 p0(std::cos(a0) * radii[ring] * height,
                               heights[ring] * height,
                               std::sin(a0) * radii[ring] * height);
            const glm::vec3 p1(std::cos(a1) * radii[ring] * height,
                               heights[ring] * height,
                               std::sin(a1) * radii[ring] * height);
            const glm::vec3 p2(std::cos(a1) * radii[ring + 1] * height,
                               heights[ring + 1] * height,
                               std::sin(a1) * radii[ring + 1] * height);
            const glm::vec3 p3(std::cos(a0) * radii[ring + 1] * height,
                               heights[ring + 1] * height,
                               std::sin(a0) * radii[ring + 1] * height);
            quad(mesh, p0, p1, p2, p3, trunk, 0.92f, 0.0f, 0.15f);
        }
    }
}

void appendFoliageTier(VegetationMeshData& mesh, float centerY, float radius,
                       float thickness, int segments, float angularOffset,
                       std::uint32_t seed, float treeHeight)
{
    Random rng{seed};
    const float phase = rng.range(0.0f, 2.0f * Pi);
    const int notchA = static_cast<int>(rng.unit() * segments) % segments;
    const int notchB = (notchA + 2 + static_cast<int>(rng.unit() * (segments - 3))) % segments;
    std::vector<float> irregular(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i)
    {
        const float angle = angularOffset + 2.0f * Pi * static_cast<float>(i) / segments;
        float r = 1.0f + 0.12f * std::sin(angle * 3.0f + phase) + rng.range(-0.08f, 0.08f);
        if (i == notchA || i == notchB)
            r *= rng.range(0.70f, 0.88f);
        irregular[static_cast<std::size_t>(i)] = r;
    }
    const glm::vec3 dark(0.025f, 0.070f, 0.018f);
    const glm::vec3 base(0.045f, 0.120f, 0.030f);
    const glm::vec3 tip(0.070f, 0.165f, 0.040f);
    const float normalizedHeight = glm::clamp(centerY / treeHeight, 0.0f, 1.0f);
    const glm::vec3 upperColor = glm::mix(base, tip, normalizedHeight * 0.72f);
    for (int i = 0; i < segments; ++i)
    {
        const int j = (i + 1) % segments;
        const float a0 = angularOffset + 2.0f * Pi * static_cast<float>(i) / segments;
        const float a1 = angularOffset + 2.0f * Pi * static_cast<float>(j) / segments;
        const auto radial = [](float a) { return glm::vec3(std::cos(a), 0.0f, std::sin(a)); };
        const glm::vec3 top0 = radial(a0) * radius * irregular[i] * 0.24f +
                               glm::vec3(0.0f, centerY + thickness * 0.42f, 0.0f);
        const glm::vec3 top1 = radial(a1) * radius * irregular[j] * 0.24f +
                               glm::vec3(0.0f, centerY + thickness * 0.42f, 0.0f);
        const glm::vec3 outer0 = radial(a0) * radius * irregular[i] +
                                 glm::vec3(0.0f, centerY, 0.0f);
        const glm::vec3 outer1 = radial(a1) * radius * irregular[j] +
                                 glm::vec3(0.0f, centerY, 0.0f);
        const glm::vec3 lower0 = radial(a0) * radius * irregular[i] * 0.56f +
                                 glm::vec3(0.0f, centerY - thickness * 0.58f, 0.0f);
        const glm::vec3 lower1 = radial(a1) * radius * irregular[j] * 0.56f +
                                 glm::vec3(0.0f, centerY - thickness * 0.58f, 0.0f);
        quad(mesh, top0, outer0, outer1, top1, upperColor, 0.86f,
             0.15f + normalizedHeight * 0.12f, 1.0f, 0.28f);
        quad(mesh, outer0, lower0, lower1, outer1, dark, 0.92f,
             0.12f + normalizedHeight * 0.10f, 1.0f, 0.22f);
    }
}

void appendGroundedIrregularCone(VegetationMeshData& mesh, float height,
                                 float radius, int segments,
                                 std::uint32_t seed)
{
    Random rng{seed};
    const float phase = rng.range(0.0f, 2.0f * Pi);
    const glm::vec3 dark(0.025f, 0.070f, 0.018f);
    const glm::vec3 base(0.045f, 0.120f, 0.030f);
    const glm::vec3 tip(0.070f, 0.165f, 0.040f);
    std::vector<float> irregular(static_cast<std::size_t>(segments));
    for (int i = 0; i < segments; ++i)
    {
        const float angle = 2.0f * Pi * static_cast<float>(i) / segments;
        irregular[i] = 1.0f + 0.12f * std::sin(angle * 3.0f + phase) +
                       rng.range(-0.08f, 0.08f);
        if (i == static_cast<int>(seed % static_cast<std::uint32_t>(segments)))
            irregular[i] *= 0.78f;
    }
    const glm::vec3 apex(rng.range(-0.035f, 0.035f) * height,
                         height * 0.96f,
                         rng.range(-0.035f, 0.035f) * height);
    for (int i = 0; i < segments; ++i)
    {
        const int j = (i + 1) % segments;
        const float a0 = 2.0f * Pi * static_cast<float>(i) / segments;
        const float a1 = 2.0f * Pi * static_cast<float>(j) / segments;
        const glm::vec3 r0(std::cos(a0), 0.0f, std::sin(a0));
        const glm::vec3 r1(std::cos(a1), 0.0f, std::sin(a1));
        const glm::vec3 p0 = r0 * radius * irregular[i] +
                             glm::vec3(0.0f, -height * 0.025f, 0.0f);
        const glm::vec3 p1 = r1 * radius * irregular[j] +
                             glm::vec3(0.0f, -height * 0.025f, 0.0f);
        const glm::vec3 m0 = r0 * radius * irregular[i] * 0.48f +
                             glm::vec3(0.0f, height * 0.58f, 0.0f);
        const glm::vec3 m1 = r1 * radius * irregular[j] * 0.48f +
                             glm::vec3(0.0f, height * 0.58f, 0.0f);
        quad(mesh, p0, p1, m1, m0, glm::mix(dark, base, 0.45f),
             0.91f, 0.12f, 1.0f, 0.24f);
        triangle(mesh, m0, m1, apex, tip, 0.87f, 0.18f, 1.0f, 0.28f);
    }
}

VegetationMeshData makeTree(float height, float crownRadius, int tiers,
                            int segments, std::uint32_t seed, bool trunk,
                            bool oneTier)
{
    VegetationMeshData mesh;
    if (oneTier)
    {
        appendGroundedIrregularCone(mesh, height, crownRadius, segments, seed);
        mesh.updateBounds();
        return mesh;
    }
    if (trunk)
        appendTaperedPrism(mesh, height, std::max(5, segments));
    Random rng{seed};
    const int actualTiers = oneTier ? 1 : tiers;
    for (int tier = 0; tier < actualTiers; ++tier)
    {
        const float t = actualTiers == 1 ? 0.43f : static_cast<float>(tier) /
            static_cast<float>(actualTiers - 1);
        const float y = glm::mix(height * 0.16f, height * 0.88f, t);
        const float envelope = oneTier ? 0.82f : std::pow(1.0f - t, 0.62f);
        const float radius = crownRadius * (0.22f + 0.78f * envelope) *
                             rng.range(0.92f, 1.08f);
        const float thickness = height * glm::mix(0.13f, 0.075f, t);
        appendFoliageTier(mesh, y, radius, thickness, segments,
                          rng.range(0.0f, 2.0f * Pi), seed + tier * 7919u, height);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet treeSet(float h, float r, int tiers, std::uint32_t seed)
{
    VegetationMeshLODSet result;
    result.lod0 = makeTree(h, r, tiers, 8, seed, true, false);
    result.lod1 = makeTree(h, r, std::max(4, tiers - 2), 6, seed, true, false);
    result.lod2 = makeTree(h, r, 1, 6, seed, false, true);
    result.shadow = makeTree(h, r, 1, 6, seed + 17u, false, true);
    return result;
}

constexpr std::array<glm::vec3, 12> IcosaVertices = {{
    {-0.525731f, 0.850651f, 0.0f}, {0.525731f, 0.850651f, 0.0f},
    {-0.525731f,-0.850651f, 0.0f}, {0.525731f,-0.850651f, 0.0f},
    {0.0f,-0.525731f, 0.850651f}, {0.0f, 0.525731f, 0.850651f},
    {0.0f,-0.525731f,-0.850651f}, {0.0f, 0.525731f,-0.850651f},
    {0.850651f,0.0f,-0.525731f}, {0.850651f,0.0f,0.525731f},
    {-0.850651f,0.0f,-0.525731f}, {-0.850651f,0.0f,0.525731f}
}};
constexpr std::array<std::array<int, 3>, 20> IcosaFaces = {{
    {{0,11,5}},{{0,5,1}},{{0,1,7}},{{0,7,10}},{{0,10,11}},
    {{1,5,9}},{{5,11,4}},{{11,10,2}},{{10,7,6}},{{7,1,8}},
    {{3,9,4}},{{3,4,2}},{{3,2,6}},{{3,6,8}},{{3,8,9}},
    {{4,9,5}},{{2,4,11}},{{6,2,10}},{{8,6,7}},{{9,8,1}}
}};

void appendLobe(VegetationMeshData& mesh, const glm::vec3& center,
                const glm::vec3& scale, float warpPhase, float wind)
{
    const glm::vec3 bottom(0.035f, 0.080f, 0.020f);
    const glm::vec3 top(0.100f, 0.190f, 0.045f);
    for (const auto& face : IcosaFaces)
    {
        glm::vec3 p[3];
        for (int k = 0; k < 3; ++k)
        {
            glm::vec3 u = IcosaVertices[face[k]];
            if (u.y < 0.0f) u.y *= 0.25f;
            const float warp = 1.0f + 0.10f * std::sin(
                u.x * 5.3f + u.y * 3.7f + u.z * 4.1f + warpPhase);
            p[k] = center + u * scale * warp;
        }
        const float y = glm::clamp(((p[0].y + p[1].y + p[2].y) / 3.0f -
                       center.y) / std::max(scale.y, 0.01f) * 0.5f + 0.5f,
                       0.0f, 1.0f);
        triangle(mesh, p[0], p[1], p[2], glm::mix(bottom, top, y),
                 0.90f, wind * y, 1.0f, 0.18f);
    }
}

VegetationMeshData makeShrubMesh(int lobes, bool swept, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    for (int i = 0; i < lobes; ++i)
    {
        const float angle = swept ? 0.0f : 2.0f * Pi * i / lobes;
        const float along = swept ? (static_cast<float>(i) - (lobes - 1) * 0.5f) * 0.34f : 0.32f;
        const glm::vec3 center(std::cos(angle) * along, 0.30f +
            (swept ? i * 0.035f : (i == 0 ? 0.18f : 0.0f)),
            std::sin(angle) * along);
        appendLobe(mesh, center, glm::vec3(rng.range(0.42f, 0.66f),
            swept ? rng.range(0.28f, 0.42f) : rng.range(0.38f, 0.58f),
            rng.range(0.38f, 0.62f)), rng.range(0.0f, 20.0f), 0.55f);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet shrubSet(bool swept, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeShrubMesh(swept ? 4 : 5, swept, seed);
    r.lod1 = makeShrubMesh(2, swept, seed);
    r.lod2 = makeShrubMesh(1, swept, seed);
    r.shadow = r.lod2;
    return r;
}

void appendGrassBlade(VegetationMeshData& mesh, const glm::vec3& base,
                      float angle, float height, float width, float bend,
                      const glm::vec3& bendDirection)
{
    const glm::vec3 right(std::cos(angle), 0.0f, std::sin(angle));
    const glm::vec3 forward = glm::normalize(glm::vec3(-right.z, 0.0f, right.x) +
                                              bendDirection * 0.45f);
    const glm::vec3 mid = base + glm::vec3(0.0f, height * 0.58f, 0.0f) +
                          forward * bend * 0.35f;
    const glm::vec3 tip = base + glm::vec3(0.0f, height, 0.0f) + forward * bend;
    const glm::vec3 normal = glm::normalize(forward * 0.35f + glm::vec3(0.0f, 0.65f, 0.0f));
    const glm::vec3 root(0.025f, 0.065f, 0.012f);
    const glm::vec3 middle(0.060f, 0.160f, 0.025f);
    const glm::vec3 top(0.115f, 0.260f, 0.045f);
    const std::uint16_t first = static_cast<std::uint16_t>(mesh.vertices.size());
    vertex(mesh, base - right * width, normal, root, 0.96f, 0.0f, 0.65f);
    vertex(mesh, base + right * width, normal, root, 0.96f, 0.0f, 0.65f);
    vertex(mesh, mid - right * width * 0.72f, normal, middle, 0.94f, 0.48f, 0.72f);
    vertex(mesh, mid + right * width * 0.72f, normal, middle, 0.94f, 0.48f, 0.72f);
    vertex(mesh, tip, normal, top, 0.92f, 1.0f, 0.80f);
    mesh.indices.insert(mesh.indices.end(), {first, static_cast<std::uint16_t>(first + 1u),
        static_cast<std::uint16_t>(first + 3u), first,
        static_cast<std::uint16_t>(first + 3u), static_cast<std::uint16_t>(first + 2u),
        static_cast<std::uint16_t>(first + 2u), static_cast<std::uint16_t>(first + 3u),
        static_cast<std::uint16_t>(first + 4u)});
}

VegetationMeshData makeGrass(int blades, int style, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    for (int i = 0; i < blades; ++i)
    {
        const float angle = 2.0f * Pi * (static_cast<float>(i) / blades) + rng.range(-0.22f, 0.22f);
        glm::vec3 commonBend(0.0f);
        if (style == 2) commonBend = glm::vec3(0.85f, 0.0f, 0.32f);
        // A tuft represents a small patch rather than one hair-thin plant.
        // The terrain spans 8.2 km, so sub-decimetre blades disappear even in
        // the near field. Keep them recognisably low-poly but readable.
        const float radial = style == 1 ? rng.range(0.42f, 1.10f) : rng.range(0.18f, 0.88f);
        appendGrassBlade(mesh, glm::vec3(std::cos(angle) * radial, -0.015f,
            std::sin(angle) * radial), angle, rng.range(1.18f, 2.24f) *
            (i == blades - 1 ? 0.78f : 1.0f), rng.range(0.055f, 0.115f),
            rng.range(style == 0 ? 0.18f : 0.26f, style == 2 ? 0.52f : 0.42f), commonBend);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet grassSet(int style, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeGrass(style == 0 ? 12 : 10, style, seed);
    r.lod1 = makeGrass(8, style, seed);
    r.lod2 = makeGrass(5, style, seed);
    r.shadow = r.lod2;
    return r;
}

void appendStem(VegetationMeshData& mesh, float height, const glm::vec3& lean,
                const glm::vec3& base)
{
    const glm::vec3 stem(0.040f, 0.150f, 0.025f);
    for (int i = 0; i < 3; ++i)
    {
        const float a0 = 2.0f * Pi * i / 3.0f;
        const float a1 = 2.0f * Pi * (i + 1) / 3.0f;
        const glm::vec3 p0 = base + glm::vec3(std::cos(a0) * 0.028f, -0.06f,
                                               std::sin(a0) * 0.028f);
        const glm::vec3 p1 = base + glm::vec3(std::cos(a1) * 0.028f, -0.06f,
                                               std::sin(a1) * 0.028f);
        const glm::vec3 top0 = base + glm::vec3(std::cos(a0) * 0.028f, height,
                                                std::sin(a0) * 0.028f) + lean;
        const glm::vec3 top1 = base + glm::vec3(std::cos(a1) * 0.028f, height,
                                                std::sin(a1) * 0.028f) + lean;
        quad(mesh, p0, p1, top1, top0,
             stem, 0.94f, 0.55f, 0.65f, 0.45f);
    }
}

void appendPetal(VegetationMeshData& mesh, const glm::vec3& center, float angle,
                 float length, float width, const glm::vec3& color, bool down)
{
    const glm::vec3 d(std::cos(angle), down ? -0.35f : 0.18f, std::sin(angle));
    const glm::vec3 s(-std::sin(angle), 0.0f, std::cos(angle));
    const glm::vec3 inner = center + d * length * 0.10f;
    const glm::vec3 outer = center + d * length;
    quad(mesh, inner - s * width * 0.25f, inner + s * width * 0.25f,
         outer + s * width, outer - s * width, color, 0.72f, 1.0f, 0.75f, 0.65f);
}

void appendOctahedron(VegetationMeshData& mesh, const glm::vec3& center,
                      float radius, const glm::vec3& color, float wind)
{
    const glm::vec3 top = center + glm::vec3(0.0f, radius, 0.0f);
    const glm::vec3 bottom = center - glm::vec3(0.0f, radius, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        const float a0 = 2.0f * Pi * i / 4.0f;
        const float a1 = 2.0f * Pi * (i + 1) / 4.0f;
        const glm::vec3 p0 = center + glm::vec3(std::cos(a0) * radius, 0.0f, std::sin(a0) * radius);
        const glm::vec3 p1 = center + glm::vec3(std::cos(a1) * radius, 0.0f, std::sin(a1) * radius);
        triangle(mesh, top, p0, p1, color, 0.74f, wind, 0.75f, 0.35f);
        triangle(mesh, bottom, p1, p0, color, 0.74f, wind, 0.75f, 0.35f);
    }
}

VegetationMeshData makeFlower(int style, bool reduced, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    const float nominalHeight = style == 2 ? 2.35f : 1.90f;
    const int plantCount = reduced ? 2 : 4;
    for (int plant = 0; plant < plantCount; ++plant)
    {
        const float angle = rng.range(0.0f, 2.0f * Pi);
        const float radius = plant == 0 ? 0.0f : rng.range(0.42f, 1.18f);
        const glm::vec3 base(std::cos(angle) * radius, 0.0f,
                             std::sin(angle) * radius);
        const float h = nominalHeight * rng.range(0.84f, 1.18f);
        const glm::vec3 lean = style == 1
            ? glm::vec3(0.28f, -0.05f, 0.08f) * rng.range(0.78f, 1.22f)
            : glm::vec3(rng.range(-0.06f, 0.06f), 0.0f,
                        rng.range(-0.06f, 0.06f));
        appendStem(mesh, h, lean, base);

        if (style == 0)
        {
            const glm::vec3 color = rng.unit() > 0.45f
                ? glm::vec3(0.92f, 0.58f, 0.025f)
                : glm::vec3(0.96f, 0.86f, 0.72f);
            const int petals = reduced ? 3 : 5;
            const glm::vec3 center = base + glm::vec3(0.0f, h, 0.0f) + lean;
            for (int i = 0; i < petals; ++i)
                appendPetal(mesh, center, 2.0f * Pi * i / petals,
                            0.40f, 0.160f, color, false);
            appendOctahedron(mesh, center + glm::vec3(0.0f, 0.032f, 0.0f),
                             0.088f, glm::vec3(0.46f, 0.21f, 0.010f), 1.0f);
        }
        else if (style == 1)
        {
            const glm::vec3 center = base + glm::vec3(0.0f, h, 0.0f) + lean;
            const int petals = reduced ? 3 : 5;
            for (int i = 0; i < petals; ++i)
                appendPetal(mesh, center, 2.0f * Pi * i / petals,
                            0.38f, 0.150f,
                            glm::vec3(0.66f, 0.055f, 0.72f), true);
        }
        else
        {
            const int heads = reduced ? 2 : 3;
            for (int i = 0; i < heads; ++i)
                appendOctahedron(mesh, base + lean + glm::vec3(
                    (i & 1) ? 0.055f : -0.040f,
                    h - 0.13f + i * 0.19f, 0.0f),
                    0.118f - i * 0.010f,
                    glm::vec3(0.74f, 0.10f, 0.64f), 1.0f);
        }
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet flowerSet(int style, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeFlower(style, false, seed);
    r.lod1 = makeFlower(style, true, seed);
    r.lod2 = VegetationMeshData{};
    r.shadow = VegetationMeshData{};
    return r;
}

VegetationMeshData makeCushion(int lobes, bool elongated, std::uint32_t seed)
{
    VegetationMeshData mesh;
    Random rng{seed};
    for (int i = 0; i < lobes; ++i)
    {
        const float a = 2.0f * Pi * i / lobes + rng.range(-0.3f, 0.3f);
        appendLobe(mesh, glm::vec3(std::cos(a) * 0.18f, 0.08f,
            std::sin(a) * 0.18f), glm::vec3(elongated ? 0.38f : 0.31f,
            0.13f, elongated ? 0.23f : 0.31f), rng.range(0.0f, 30.0f), 0.08f);
    }
    const glm::vec3 edge(0.100f, 0.120f, 0.040f);
    const int leaves = std::max(6, lobes * 2);
    for (int i = 0; i < leaves; ++i)
    {
        const float a = 2.0f * Pi * i / leaves;
        const glm::vec3 d(std::cos(a), 0.0f, std::sin(a));
        const glm::vec3 s(-d.z, 0.0f, d.x);
        triangle(mesh, d * 0.18f - s * 0.035f, d * 0.50f,
                 d * 0.18f + s * 0.035f, edge, 0.94f, 0.04f, 0.55f, 0.65f);
    }
    mesh.updateBounds();
    return mesh;
}

VegetationMeshLODSet cushionSet(bool elongated, std::uint32_t seed)
{
    VegetationMeshLODSet r;
    r.lod0 = makeCushion(4, elongated, seed);
    r.lod1 = makeCushion(2, elongated, seed);
    r.lod2 = makeCushion(1, elongated, seed);
    r.shadow = r.lod2;
    return r;
}

VegetationMeshLODSet externalSet(const std::string& assetName,
                                 float targetHeight, float windStrength,
                                 float shapeVariation,
                                 VegetationMeshLODSet fallback)
{
    const std::filesystem::path directory = vegetationAssetDirectory();
    const auto load = [&](const char* suffix, bool loadRuntimeImages,
                          VegetationMeshData fallbackMesh)
    {
        const std::filesystem::path path =
            directory / (assetName + "_" + suffix + ".glb");
        if (const auto loaded = loadVegetationAsset(
                path, assetName, targetHeight, windStrength, shapeVariation,
                loadRuntimeImages))
            return *loaded;
        std::cerr << "Using procedural vegetation fallback for "
                  << path.filename().string() << std::endl;
        return fallbackMesh;
    };

    VegetationMeshLODSet result;
    result.lod0 = load("lod0", true, std::move(fallback.lod0));
    result.lod1 = load("lod1", false, std::move(fallback.lod1));
    result.lod2 = load("lod2", false, std::move(fallback.lod2));
    result.shadow = load("shadow", false, std::move(fallback.shadow));
    // All four LOD exports reference the same material images. Keep one CPU
    // copy on LOD0; the GPU upload shares those texture objects with the other
    // LOD VAOs while their per-vertex texture indices remain intact.
    result.lod1.baseColorTextures.clear();
    result.lod1.normalTextures.clear();
    result.lod2.baseColorTextures.clear();
    result.lod2.normalTextures.clear();
    result.shadow.baseColorTextures.clear();
    result.shadow.normalTextures.clear();
    result.lod1.baseColorAtlasMips.clear();
    result.lod1.normalAtlasMips.clear();
    result.lod1.foliageDataAtlasMips.clear();
    result.lod2.baseColorAtlasMips.clear();
    result.lod2.normalAtlasMips.clear();
    result.lod2.foliageDataAtlasMips.clear();
    result.shadow.baseColorAtlasMips.clear();
    result.shadow.normalAtlasMips.clear();
    result.shadow.foliageDataAtlasMips.clear();
    return result;
}
}

void VegetationMeshData::updateBounds()
{
    if (vertices.empty()) { boundsMin = boundsMax = glm::vec3(0.0f); return; }
    boundsMin = glm::vec3(std::numeric_limits<float>::max());
    boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const auto& v : vertices)
    {
        boundsMin = glm::min(boundsMin, v.position);
        boundsMax = glm::max(boundsMax, v.position);
    }
}

VegetationMeshData AlpineVegetationMeshFactory::makeShowcaseAsset(
    const std::string& assetName, float targetHeight, float windStrength,
    float shapeVariation)
{
    const std::filesystem::path path =
        bakedVegetationAssetDirectory() / assetName / (assetName + ".glb");
    if (const auto loaded = loadVegetationAsset(
            path, assetName, targetHeight, windStrength, shapeVariation,
            false))
        return *loaded;
    std::cerr << "High-detail vegetation showcase asset unavailable: "
              << path.string() << std::endl;
    // The material lab intentionally has no low-poly fallback.
    return VegetationMeshData{};
}

VegetationMeshLODSet AlpineVegetationMeshFactory::makeTallConifer(std::uint32_t s)
{
    return externalSet("picea_tall", 45.0f, 0.34f, 0.82f,
                       treeSet(45.0f, 11.5f, 7, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeBroadConifer(std::uint32_t s)
{
    return externalSet("larix_broad", 34.0f, 0.42f, 0.88f,
                       treeSet(34.0f, 13.0f, 6, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeSapling(std::uint32_t s)
{
    return externalSet("larix_sapling", 18.0f, 0.48f, 0.92f,
                       treeSet(18.0f, 5.2f, 4, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeRoundShrub(std::uint32_t s) { return shrubSet(false, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeWindSweptShrub(std::uint32_t s) { return shrubSet(true, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeGrassTuftA(std::uint32_t s)
{
    return externalSet("grass_meadow", 3.2f, 0.88f, 0.72f, grassSet(0, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeGrassTuftB(std::uint32_t s)
{
    return externalSet("grass_seedhead", 3.2f, 0.94f, 0.82f, grassSet(1, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeGrassTuftC(std::uint32_t s)
{
    return externalSet("grass_meadow", 3.2f, 0.82f, 0.92f, grassSet(2, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeStarFlower(std::uint32_t s)
{
    return externalSet("flower_yellow", 4.2f, 0.72f, 0.58f, flowerSet(0, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeBellFlower(std::uint32_t s)
{
    return externalSet("flower_bell", 4.2f, 0.78f, 0.62f, flowerSet(1, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeSpikeFlower(std::uint32_t s)
{
    return externalSet("flower_white", 4.8f, 0.74f, 0.55f, flowerSet(2, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makePinkFlower(std::uint32_t s)
{
    return externalSet("flower_pink", 3.8f, 0.78f, 0.60f, flowerSet(1, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeCrocusFlower(std::uint32_t s)
{
    return externalSet("flower_crocus", 3.6f, 0.74f, 0.56f, flowerSet(0, s));
}
VegetationMeshLODSet AlpineVegetationMeshFactory::makeCushionPlantA(std::uint32_t s) { return cushionSet(false, s); }
VegetationMeshLODSet AlpineVegetationMeshFactory::makeCushionPlantB(std::uint32_t s) { return cushionSet(true, s); }
