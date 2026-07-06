#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "rendering/modelload/Mesh.h"

// processNode递归遍历节点
// processMesh遍历节点中的mesh
// loadMaterialTextures遍历材质的所有纹理，加载到纹理数组里

// Model构造函数,负责加载模型，里面包含processNode,processMesh,loadMaterialTextures整个流程
class Model
{
public:
    explicit Model(const std::string& path);

    void Draw(Shader& shader) const;
    void Draw(Shader& shader, const glm::mat4& transform) const;

private:
    std::vector<Mesh> meshes;
    std::string directory;

    // path -> 已加载的 Texture，避免同一张贴图被多个 mesh 重复上传 GPU
    std::unordered_map<std::string, Texture> textureCache;

    void loadModel(const std::string& path);
    // 第二个参数: processNode可以读取scene,但不能修改scene
    // 递归处理节点和子节点
    void processNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
    // 遍历节点中的mesh
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& nodeTransform);

    MaterialFlags loadMaterialFlags(aiMaterial* mat);

    // 处理单个材质类型下的所有纹理
    // scene: 内嵌纹理(GLB 里 "*N" 引用) 需要靠它才能取到 scene->mTextures[]
    // isColorData: true = sRGB 编码颜色数据(baseColor/emissive)，false = 线性数据(normal/metallic/roughness/ao)
    std::vector<Texture> loadMaterialTextures(
        aiMaterial* mat,
        aiTextureType type,
        const std::string& typeName,
        const aiScene* scene,
        bool isColorData);
};
