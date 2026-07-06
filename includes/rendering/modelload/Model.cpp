#include "rendering/modelload/Model.h"

#include <assimp/GltfMaterial.h>
#include <stb_image.h>
#include <iostream>
#include <stdexcept>

namespace
{
glm::mat4 toGlm(const aiMatrix4x4& m)
{
    glm::mat4 result(1.0f);
    result[0][0] = m.a1; result[1][0] = m.a2; result[2][0] = m.a3; result[3][0] = m.a4;
    result[0][1] = m.b1; result[1][1] = m.b2; result[2][1] = m.b3; result[3][1] = m.b4;
    result[0][2] = m.c1; result[1][2] = m.c2; result[2][2] = m.c3; result[3][2] = m.c4;
    result[0][3] = m.d1; result[1][3] = m.d2; result[2][3] = m.d3; result[3][3] = m.d4;
    return result;
}
}

Model::Model(const std::string& path)
{
    loadModel(path);
}

void Model::Draw(Shader& shader) const
{
    Draw(shader, glm::mat4(1.0f));
}

void Model::Draw(Shader& shader, const glm::mat4& transform) const
{
    for (const auto& mesh : meshes)
        mesh.Draw(shader, transform);
}

void Model::loadModel(const std::string& path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene, glm::mat4(1.0f));
}

void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform)
{
    glm::mat4 nodeTransform = parentTransform * toGlm(node->mTransformation);

    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        // 保存每个网格的索引编号
        unsigned int meshIndex = node->mMeshes[i];
        // 从scene的mMeshes数组中获取对应索引的aiMesh对象指针
        aiMesh* mesh = scene->mMeshes[meshIndex];
        // 将aimesh转化为你自己的mesh,并保存到meshes数组中
        meshes.push_back(processMesh(mesh, scene, nodeTransform));
    }

    // 递归处理所有子节点
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
        processNode(node->mChildren[i], scene, nodeTransform);
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeTransform)
{
    std::vector<Mesh::Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces * 3);

    // 1. 处理顶点数据
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        Mesh::Vertex vertex{};

        vertex.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        if (mesh->HasNormals())
            vertex.Normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

        if (mesh->mTextureCoords[0])
        {
            vertex.TexCoords = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.Tangent   = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
                vertex.Bitangent = { mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
            }
        }
        else
        {
            vertex.TexCoords = { 0.0f, 0.0f };
        }

        // 新增：读取第二套 UV（不是所有 mesh 都有，要判空）
        if (mesh->mTextureCoords[1])
            vertex.TexCoords1 = { mesh->mTextureCoords[1][i].x, mesh->mTextureCoords[1][i].y };
        else
            vertex.TexCoords1 = vertex.TexCoords;   // 没有就退化成用 UV0，至少不会是垃圾值

        vertices.push_back(vertex);
    }

    // 2. 处理索引数据
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // 3. 处理材质数据（贴图 + 标量属性）
    MaterialFlags flags;   // 默认值：单面、不透明，即使没有材质也安全

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        auto baseColorMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_baseColor", scene, true);
        textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());

        auto normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene, false);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        auto metalnessMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", scene, false);
        textures.insert(textures.end(), metalnessMaps.begin(), metalnessMaps.end());

        auto roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", scene, false);
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

        auto metallicRoughnessMaps = loadMaterialTextures(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, "texture_metallicRoughness", scene, false);
        textures.insert(textures.end(), metallicRoughnessMaps.begin(), metallicRoughnessMaps.end());

        auto aoMaps = loadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_ao", scene, false);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

        auto emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_emissive", scene, true);
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

        // 兼容旧 Phong 槽位（非 glTF 模型）
        auto diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene, true);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        auto specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene, true);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        flags = loadMaterialFlags(material);
    }

    return Mesh(std::move(vertices), std::move(indices), std::move(textures), flags, nodeTransform);
}
MaterialFlags Model::loadMaterialFlags(aiMaterial* mat)
{
    MaterialFlags flags;

    // doubleSided：通用 key，非 glTF 格式材质也可能有
    mat->Get(AI_MATKEY_TWOSIDED, flags.doubleSided);

    // alphaMode：glTF 专有，返回字符串 "OPAQUE" / "MASK" / "BLEND"
    aiString alphaMode;
    if (mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        flags.alphaMask = (std::string(alphaMode.C_Str()) == "MASK");

    // alphaCutoff：glTF 专有，MASK 模式下的判定阈值
    mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, flags.alphaCutoff);

    aiColor4D baseColor;
    if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
    {
        flags.baseColorFactor = glm::vec4(
            baseColor.r,
            baseColor.g,
            baseColor.b,
            baseColor.a);
    }
    else
    {
        aiColor3D diffuseColor;
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
        {
            flags.baseColorFactor = glm::vec4(
                diffuseColor.r,
                diffuseColor.g,
                diffuseColor.b,
                1.0f);
        }
    }

    mat->Get(AI_MATKEY_METALLIC_FACTOR, flags.metallicFactor);
    mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, flags.roughnessFactor);

    return flags;
}

// 返回textureID
// 纹理在scene -> mTextures数组里，但这一版还没处理内嵌纹理，只走外部文件路径
// 遍历材质的所有纹理，加载到纹理数组里
std::vector<Texture> Model::loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& typeName,
    const aiScene* scene,
    bool isColorData)
{
    std::vector<Texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); ++i)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::cout << str.C_Str() << std::endl;

        std::string key = str.C_Str();
        bool isEmbedded = !key.empty() && key[0] == '*';

        std::string cacheKey = isEmbedded ? key : (directory + '/' + key);

        auto cached = textureCache.find(cacheKey);
        if (cached != textureCache.end())
        {
            textures.push_back(cached->second);
            continue;
        }

        int width = 0, height = 0, channels = 0;
        unsigned char* data = nullptr;
        bool ownsData = true; // true -> stbi_image_free 释放；false -> delete[] 释放

        if (isEmbedded)
        {
            int index = -1;
            try
            {
                index = std::stoi(key.substr(1)); // "*3" -> 3
            }
            catch (const std::exception&)
            {
                std::cerr << "Invalid embedded texture key: " << key << std::endl;
                continue;
            }

            if (index < 0 || static_cast<unsigned int>(index) >= scene->mNumTextures)
            {
                std::cerr << "Embedded texture index out of range: " << key << std::endl;
                continue;
            }

            const aiTexture* embeddedTex = scene->mTextures[index];

            if (embeddedTex->mHeight == 0)
            {
                // 压缩格式(PNG/JPEG)，pcData 是原始文件字节流，长度是 mWidth
                data = stbi_load_from_memory(
                    reinterpret_cast<unsigned char*>(embeddedTex->pcData),
                    embeddedTex->mWidth,
                    &width, &height, &channels, 0);
            }
            else
            {
                // 未压缩的原始像素(aiTexel，BGRA 顺序)，重排为 RGBA
                width = embeddedTex->mWidth;
                height = embeddedTex->mHeight;
                channels = 4;
                data = new unsigned char[width * height * 4];
                for (int p = 0; p < width * height; ++p)
                {
                    data[p * 4 + 0] = embeddedTex->pcData[p].r;
                    data[p * 4 + 1] = embeddedTex->pcData[p].g;
                    data[p * 4 + 2] = embeddedTex->pcData[p].b;
                    data[p * 4 + 3] = embeddedTex->pcData[p].a;
                }
                ownsData = false;
            }
        }
        else
        {
            data = stbi_load(cacheKey.c_str(), &width, &height, &channels, 0);
        }

        Texture texture{};
        texture.type = typeName;
        texture.path = cacheKey;

        if (data)
        {
            // sourceFormat：告诉 GL 传入 data 的实际通道排布，必须和 channels 对应
            GLenum sourceFormat = channels == 1 ? GL_RED
                     : channels == 2 ? GL_RG
                     : channels == 3 ? GL_RGB
                     : GL_RGBA;

            // internalFormat：GPU 内部存储格式，颜色数据需要 sRGB 解码，线性数据不需要
            GLenum internalFormat;
            if (isColorData && channels == 4)
                internalFormat = GL_SRGB_ALPHA;
            else if (isColorData && channels == 3)
                internalFormat = GL_SRGB;
            else
                internalFormat = sourceFormat;

            GLint previousUnpackAlignment = 4;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glGenTextures(1, &texture.id);
            glBindTexture(GL_TEXTURE_2D, texture.id);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, sourceFormat, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            if (isColorData && channels == 2)
            {
                GLint swizzle[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
            }

            if (ownsData) stbi_image_free(data);
            else delete[] data;
        }
        else
        {
            std::cerr << "Texture failed to load: " << cacheKey << std::endl;
            texture.id = 0;
        }

        if (texture.id != 0)
        {
            textureCache[cacheKey] = texture;
            textures.push_back(texture);
        }
        else
        {
            if (ownsData && data) stbi_image_free(data);
            else if (!ownsData && data) delete[] data;
        }
    }

    return textures;
}
