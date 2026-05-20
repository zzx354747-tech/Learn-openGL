#include "Model.h"
#include "stb_image.h"

void Model::loadModel(const std::string& path)
{
    // 创建Assimp导入器实例
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | 
        // 如果没有法线数据，自动生成
        aiProcess_GenSmoothNormals |
        // 反转y轴的纹理坐标，因为OpenGL的纹理坐标原点在左下角，而许多模型文件的纹理坐标原点在左上角
        aiProcess_FlipUVs);

        // 检查导入是否成功
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

    // 从第0个字符开始，找到最后一个'/'字符的位置，并提取目录路径
    directory = path.substr(0, path.find_last_of('/'));

    // 第一个参数：根据scene指针找到aiScene对象，再访问它里面的mRootNode成员，得到根节点的指针
    // 第二个参数：整个场景的指针
    processNode(scene->mRootNode, scene);
}

    // 第二个参数: processNode可以读取scene,但不能修改scene
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

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<texture> textures;

    // 1. 处理顶点数据
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertex v;

        v.position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

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

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<texture> diffuseMaps = loadMaterialTextures(
            material,
            aiTextureType_DIFFUSE,
            "texture_diffuse",
            scene
        );
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<texture> specularMaps = loadMaterialTextures(
            material,
            aiTextureType_SPECULAR,
            "texture_specular",
            scene
        );
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    return Mesh(vertices, indices, textures);
}

void Model::draw(Shader& shader)
{
    for (Mesh& mesh : meshes)
    {
        mesh.draw(shader);
    }
}

unsigned int TextureFromFile(const char* path, const std::string& directory, const aiScene* scene)
{
    std::string filename = std::string(path);

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
                data = stbi_load_from_memory(
                    reinterpret_cast<unsigned char*>(aiTex->pcData),
                    aiTex->mWidth,
                    &width,
                    &height,
                    &nrComponents,
                    0
                );
            }
            // 非压缩格式，Assimp 已经解成像素
            else
            {
                width = aiTex->mWidth;
                height = aiTex->mHeight;
                nrComponents = 4;

                glBindTexture(GL_TEXTURE_2D, textureID);

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

                glGenerateMipmap(GL_TEXTURE_2D);

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

        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);

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

        glGenerateMipmap(GL_TEXTURE_2D);

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
        mat->GetTexture(type, i, &str);

        texture tex;
        tex.id = TextureFromFile(str.C_Str(), directory, scene);
        tex.type = typeName;
        tex.path = str.C_Str();

        textures.push_back(tex);
    }

    return textures;
}