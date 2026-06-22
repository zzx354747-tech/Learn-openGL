#pragma once

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/GltfMaterial.h>
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
    glm::vec3 getBoundsMin() const { return boundsMin; }
    glm::vec3 getBoundsMax() const { return boundsMax; }
    glm::vec3 getBoundsCenter() const { return (boundsMin + boundsMax) * 0.5f; }
    glm::vec3 getBoundsSize() const { return boundsMax - boundsMin; }
    bool hasValidBounds() const { return hasBounds; }

private:
    std::vector<Mesh> meshes;
    // 用来保存模型所在目录
    std::string directory;
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    // 已加载纹理的缓存
    std::unordered_map<std::string, texture> textures_loaded;
    bool hasBounds = false;

    std::vector<texture> loadMaterialTextures(
    aiMaterial* mat,
    aiTextureType type,
    const std::string& typeName,
    const aiScene* scene);

    void loadModel(const std::string& path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};
