#include "Model.h"
#include "stb_image.h"

#include <algorithm>
#include <chrono>

// processNode递归便利节点
// processMesh遍历节点中的mesh
// loadMaterialTextures遍历材质的所有纹理，加载到纹理数组里

// Model构造函数,负责加载模型，里面包含processNode,processMesh,loadMaterialTextures整个流程

void Model::loadModel(const std::string& path)
{
    boundsMin = glm::vec3(0.0f);
    boundsMax = glm::vec3(0.0f);
    hasBounds = false;

    Assimp::Importer importer;

    unsigned int importFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_CalcTangentSpace |
        aiProcess_PreTransformVertices;

    // ↓ 计时包裹 ReadFile
    auto t0 = std::chrono::high_resolution_clock::now();

    const aiScene* scene = importer.ReadFile(path, importFlags);

    auto t1 = std::chrono::high_resolution_clock::now();
    // ↑ 计时结束

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    // ↓ 计时包裹 processNode
    processNode(scene->mRootNode, scene);

    auto t2 = std::chrono::high_resolution_clock::now();
    // ↑ 计时结束

    auto msReadFile = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    auto msProcess  = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "ReadFile:    " << msReadFile << " ms\n";
    std::cout << "processNode: " << msProcess  << " ms\n";
}

    // 第二个参数: processNode可以读取scene,但不能修改scene
    // 递归处理节点和子节点
void Model::processNode(aiNode* node, const aiScene* scene)
{
    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // 保存每个网格的索引编号
        unsigned int meshIndex = node->mMeshes[i];
        // 从scene的mMeshes数组中获取对应索引的aiMesh对象指针
        aiMesh* mesh = scene->mMeshes[meshIndex];
        // 将aimesh转化为你自己的mesh,并保存到meshes数组中
        meshes.push_back(processMesh(mesh, scene));
    }

    // 递归处理所有子节点
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

// 遍历节点中的mesh
Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<texture> textures;
    MaterialFactors materialFactors;

    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces * 3);

    // 1. 处理顶点数据
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertex v{};

        v.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (!hasBounds)
        {
            boundsMin = v.position;
            boundsMax = v.position;
            hasBounds = true;
        }
        else
        {
            boundsMin = glm::min(boundsMin, v.position);
            boundsMax = glm::max(boundsMax, v.position);
        }

        if (mesh->HasNormals())
        {
            v.normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        if (mesh->mTextureCoords[0])
        {
            v.texCoords = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            v.texCoords = glm::vec2(0.0f, 0.0f);
        }

        if (mesh->mTextureCoords[1])
        {
            v.texCoords1 = glm::vec2(
                mesh->mTextureCoords[1][i].x,
                mesh->mTextureCoords[1][i].y
            );
        }
        else
        {
            v.texCoords1 = v.texCoords;
        }

        if (mesh->HasTangentsAndBitangents())
        {
            v.tangent = glm::vec3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            );

            v.bitangent = glm::vec3(
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            );
        }
        else
        {
            v.tangent = glm::vec3(0.0f);
            v.bitangent = glm::vec3(0.0f);
        }

        vertices.push_back(v);
    }

    // 2. 处理索引数据
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 处理材质数据
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        aiColor4D baseColor;
        if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS)
        {
            materialFactors.baseColor = glm::vec4(
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a
            );
        }
        else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS)
        {
            materialFactors.baseColor = glm::vec4(
                baseColor.r,
                baseColor.g,
                baseColor.b,
                baseColor.a
            );
        }

        material->Get(AI_MATKEY_METALLIC_FACTOR, materialFactors.metallic);
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, materialFactors.roughness);

        aiString alphaMode;
        if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        {
            materialFactors.alphaMask = std::string(alphaMode.C_Str()) == "MASK";
        }
        material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, materialFactors.alphaCutoff);

        std::vector<texture> diffuseMaps = loadMaterialTextures(
            material,
            aiTextureType_DIFFUSE,
            "texture_diffuse",
            scene
        );
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<texture> baseColorMaps = loadMaterialTextures(
            material,
            aiTextureType_BASE_COLOR,
            "texture_basecolor",
            scene
        );
        textures.insert(textures.end(), baseColorMaps.begin(), baseColorMaps.end());

        std::vector<texture> specularMaps = loadMaterialTextures(
            material,
            aiTextureType_SPECULAR,
            "texture_specular",
            scene
        );
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        std::vector<texture> normalMaps = loadMaterialTextures(
            material,
            aiTextureType_NORMALS,
            "texture_normal",
            scene
        );
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<texture> heightMaps = loadMaterialTextures(
            material,
            aiTextureType_HEIGHT,
            "texture_height",
            scene
        );
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        std::vector<texture> displacementMaps = loadMaterialTextures(
            material,
            aiTextureType_DISPLACEMENT,
            "texture_height",
            scene
        );
        textures.insert(textures.end(), displacementMaps.begin(), displacementMaps.end());

        std::vector<texture> roughnessMaps = loadMaterialTextures(
            material,
            aiTextureType_DIFFUSE_ROUGHNESS,
            "texture_roughness",
            scene
        );
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

        std::vector<texture> metallicMaps = loadMaterialTextures(
            material,
            aiTextureType_METALNESS,
            "texture_metallic",
            scene
        );
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        std::vector<texture> metallicRoughnessMaps = loadMaterialTextures(
            material,
            aiTextureType_GLTF_METALLIC_ROUGHNESS,
            "texture_metallic_roughness",
            scene
        );
        textures.insert(textures.end(), metallicRoughnessMaps.begin(), metallicRoughnessMaps.end());
    }

    return Mesh(std::move(vertices), std::move(indices), std::move(textures), materialFactors);
}

void Model::draw(Shader& shader)
{
    for (Mesh& mesh : meshes)
    {
        mesh.draw(shader);
    }
}

// 返回textureID
// 纹理在scene -> mTextures数组里， 所以需要scene指针来访问
// 处理单个纹理
unsigned int TextureFromFile(const char* path, const std::string& directory, const aiScene* scene)
{
    std::string filename = std::string(path);
    std::replace(filename.begin(), filename.end(), '\\', '/');

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = nullptr;

    // 情况 1：glb / gltf 的嵌入式纹理
    if (filename.size() > 0 && filename[0] == '*')
    {
        int textureIndex = std::stoi(filename.substr(1));

        if (scene && textureIndex >= 0 && textureIndex < scene->mNumTextures)
        {
            aiTexture* aiTex = scene->mTextures[textureIndex];

            // 压缩格式，例如 png / jpg 嵌入在 glb 里
            if (aiTex->mHeight == 0)
            {
                auto tDecodeStart = std::chrono::high_resolution_clock::now();
                data = stbi_load_from_memory(
                    reinterpret_cast<unsigned char*>(aiTex->pcData),
                    aiTex->mWidth,
                    &width,
                    &height,
                    &nrComponents,
                    0
                );
                auto tDecodeEnd = std::chrono::high_resolution_clock::now();
                std::cout << "  stbi_load_from_memory: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(tDecodeEnd - tDecodeStart).count()
                 << " ms  (" << width << "x" << height << ")\n";
            }
            // 非压缩格式，Assimp 已经解成像素
            else
            {
                width = aiTex->mWidth;
                height = aiTex->mHeight;
                nrComponents = 4;
                 auto tUploadStart = std::chrono::high_resolution_clock::now();

                glBindTexture(GL_TEXTURE_2D, textureID);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RGBA,
                    width,
                    height,
                    0,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    aiTex->pcData
                );

                auto tMipStart = std::chrono::high_resolution_clock::now();

                glGenerateMipmap(GL_TEXTURE_2D);

                auto tEnd = std::chrono::high_resolution_clock::now();

                auto msUpload = std::chrono::duration_cast<std::chrono::milliseconds>(tMipStart - tUploadStart).count();
                auto msMip    = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tMipStart).count();

                std::cout << "  [" << path << "]\n";
                std::cout << "    glTexImage2D:     " << msUpload << " ms\n";
                std::cout << "    glGenerateMipmap: " << msMip    << " ms\n";


                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                return textureID;
            }
        }
    }
    // 情况 2：普通外部贴图，例如 obj / fbx
    else
    {
        filename = directory + "/" + filename;

        data = stbi_load(
            filename.c_str(),
            &width,
            &height,
            &nrComponents,
            0
        );
    }

    if (data)
    {
        GLenum format = GL_RGB;
        bool isLuminanceAlpha = nrComponents == 2;
        auto tUploadStart = std::chrono::high_resolution_clock::now();

        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 2)
            format = GL_RG;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            width,
            height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            data
        );

        auto tMipStart = std::chrono::high_resolution_clock::now();

        if (isLuminanceAlpha)
        {
            GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
            glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        auto tEnd = std::chrono::high_resolution_clock::now();

        auto msUpload = std::chrono::duration_cast<std::chrono::milliseconds>(tMipStart - tUploadStart).count();
        auto msMip    = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tMipStart).count();

        std::cout << "  [" << path << "]\n";
        std::cout << "    glTexImage2D:     " << msUpload << " ms\n";
        std::cout << "    glGenerateMipmap: " << msMip    << " ms\n";

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// 遍历材质的所有纹理，加载到纹理数组里
std::vector<texture> Model::loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& typeName,
    const aiScene* scene
)
{
    std::vector<texture> textures;

    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        unsigned int uvIndex = 0;
        mat->GetTexture(type, i, &str, nullptr, &uvIndex);

        auto it = textures_loaded.find(str.C_Str());
        if (it != textures_loaded.end())
        {
            textures.push_back(it->second);
        }
        else
        {
            auto tDecode = std::chrono::high_resolution_clock::now();
            texture tex;
            tex.id = TextureFromFile(str.C_Str(), directory, scene);
            auto tDone = std::chrono::high_resolution_clock::now();
            std::cout << "Total texture: " 
            << std::chrono::duration_cast<std::chrono::milliseconds>(tDone - tDecode).count() 
            << " ms\n";

            tex.type = typeName;
            tex.path = str.C_Str();
            tex.uvIndex = uvIndex;

            textures.push_back(tex);
            textures_loaded[tex.path] = tex;
        }
    }

    return textures;
}