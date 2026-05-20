#pragma once

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"

class Model 
{
public:
    Model(const std::string& path)
    {
        loadModel(path);
    }

    void draw(Shader& shader);  

private:
    std::vector<Mesh> meshes;
    // 用来保存模型所在目录
    std::string directory;

    std::vector<texture> loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& typeName,
    const aiScene* scene);

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};