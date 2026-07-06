#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "core/Shader.h"

struct Texture
{
    unsigned int id;
    std::string  type;   // "texture_baseColor" / "texture_normal" / "texture_metallic" / ...
    std::string  path;   // 用于跨 mesh 去重缓存的 key
};

struct MaterialFlags
{
    bool  doubleSided = false;
    bool  alphaMask   = false;
    float alphaCutoff = 0.5f;   // glTF 规范默认值
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float roughnessFactor = 1.0f;
    float metallicFactor = 0.0f;
};

class Mesh
{
public:
    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec2 TexCoords1;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

    Mesh(std::vector<Vertex> vertices,
         std::vector<unsigned int> indices,
         std::vector<Texture> textures,
         MaterialFlags flags,
         const glm::mat4& localTransform);

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    ~Mesh();

    void Draw(Shader& shader) const;
    void Draw(Shader& shader, const glm::mat4& parentTransform) const;

    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture>      textures;
    MaterialFlags             flags;   // 新增成员
    glm::mat4                 localTransform = glm::mat4(1.0f);

private:
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    void setupMesh();
};
