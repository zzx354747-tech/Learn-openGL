#include "rendering/modelload/Mesh.h"

#include <cstddef>
#include <chrono>
#include <utility>

Mesh::Mesh(std::vector<Vertex> vertices,
           std::vector<unsigned int> indices,
           std::vector<Texture> textures,
           MaterialFlags flags,
           const glm::mat4& localTransform)
    : vertices(std::move(vertices))
    , indices(std::move(indices))
    , textures(std::move(textures))
    , flags(flags)
    , localTransform(localTransform)
{
    setupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertices(std::move(other.vertices))
    , indices(std::move(other.indices))
    , textures(std::move(other.textures))
    , flags(other.flags)
    , localTransform(other.localTransform)
    , VAO(other.VAO)
    , VBO(other.VBO)
    , EBO(other.EBO)
{
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this == &other)
        return *this;

    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);

    vertices = std::move(other.vertices);
    indices  = std::move(other.indices);
    textures = std::move(other.textures);
    flags    = other.flags;
    localTransform = other.localTransform;
    VAO = other.VAO;
    VBO = other.VBO;
    EBO = other.EBO;

    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;

    return *this;
}

Mesh::~Mesh()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords1));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
    glBindVertexArray(0);
}

void Mesh::Draw(Shader& shader) const
{
    Draw(shader, glm::mat4(1.0f));
}

void Mesh::Draw(Shader& shader, const glm::mat4& parentTransform) const
{
    static const auto windClockStart = std::chrono::steady_clock::now();
    const float windTime = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - windClockStart).count();
    shader.setBool("enableWind", flags.windAffected);
    shader.setFloat("windTime", windTime);
    shader.setFloat("windStrength", flags.windStrength);
    shader.setVec2("windDirection", flags.windDirection);

    // 按类型分别绑定，固定 uniform 名，不再编号
    bool hasBaseColor = false, hasNormal = false, hasMetallic = false, hasRoughness = false, hasParallax = false;
    bool hasTerrainBlend = false, hasTerrainGrassAlbedo = false;
    unsigned int metallicTexId = 0, roughnessTexId = 0;

    shader.setBool("hasAlbedoMap", false);
    shader.setBool("hasNormalMap", false);
    shader.setBool("hasParallaxMap", false);
    shader.setBool("hasRoughnessMap", false);
    shader.setBool("hasMetallicMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("useTerrainBlend", false);
    shader.setBool("hasTerrainGrassNormal", false);
    shader.setBool("hasTerrainGrassRoughness", false);
    shader.setBool("hasTerrainGrassMetallic", false);
    shader.setVec3("albedoColor", glm::vec3(flags.baseColorFactor));
    shader.setFloat("roughnessFactor", flags.roughnessFactor);
    shader.setFloat("metallicFactor", flags.metallicFactor);

    for (unsigned int i = 0; i < textures.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i].id);

        const std::string& type = textures[i].type;

        if (type == "texture_baseColor" || type == "texture_diffuse")
        {
            shader.setInt("albedoTexture", i);
            shader.setBool("hasAlbedoMap", true);
            hasBaseColor = true;
        }
        else if (type == "texture_normal")
        {
            shader.setInt("normalTexture", i);
            shader.setBool("hasNormalMap", true);
            hasNormal = true;
        }
        else if (type == "texture_metallicRoughness")
        {
            shader.setInt("metallicTexture", i);
            shader.setInt("roughnessTexture", i);
            shader.setBool("hasMetallicMap", true);
            shader.setBool("hasRoughnessMap", true);
            metallicTexId = textures[i].id;
            roughnessTexId = textures[i].id;
            hasMetallic = true;
            hasRoughness = true;
        }
        else if (type == "texture_metallic")
        {
            shader.setInt("metallicTexture", i);
            shader.setBool("hasMetallicMap", true);
            metallicTexId = textures[i].id;
            hasMetallic = true;
        }
        else if (type == "texture_roughness")
        {
            shader.setInt("roughnessTexture", i);
            shader.setBool("hasRoughnessMap", true);
            roughnessTexId = textures[i].id;
            hasRoughness = true;
        }
        else if (type == "texture_height")
        {
            shader.setInt("parallaxTexture", i);
            shader.setBool("hasParallaxMap", true);
            hasParallax = true;
        }
        else if (type == "texture_terrainBlend")
        {
            shader.setInt("terrainBlendTexture", i);
            hasTerrainBlend = true;
        }
        else if (type == "texture_terrainGrassAlbedo")
        {
            shader.setInt("terrainGrassAlbedo", i);
            hasTerrainGrassAlbedo = true;
        }
        else if (type == "texture_terrainGrassNormal")
        {
            shader.setInt("terrainGrassNormal", i);
            shader.setBool("hasTerrainGrassNormal", true);
        }
        else if (type == "texture_terrainGrassRoughness")
        {
            shader.setInt("terrainGrassRoughness", i);
            shader.setBool("hasTerrainGrassRoughness", true);
        }
        else if (type == "texture_terrainGrassMetallic")
        {
            shader.setInt("terrainGrassMetallic", i);
            shader.setBool("hasTerrainGrassMetallic", true);
        }
    }

    // 检测是否是 ORM 打包（metallic 和 roughness 指向同一张贴图 id）
    bool packedMR = hasMetallic && hasRoughness && (metallicTexId == roughnessTexId);
    shader.setBool("usePackedMetallicRoughness", packedMR);

    if (!hasBaseColor) shader.setBool("hasAlbedoMap", false);
    if (!hasNormal)    shader.setBool("hasNormalMap", false);
    if (!hasMetallic)  shader.setBool("hasMetallicMap", false);
    if (!hasRoughness) shader.setBool("hasRoughnessMap", false);
    if (!hasParallax)  shader.setBool("hasParallaxMap", false);
    shader.setBool("useTerrainBlend", hasTerrainBlend && hasTerrainGrassAlbedo);
    if (!hasTerrainBlend || !hasTerrainGrassAlbedo)
    {
        shader.setBool("hasTerrainGrassNormal", false);
        shader.setBool("hasTerrainGrassRoughness", false);
        shader.setBool("hasTerrainGrassMetallic", false);
    }

    glActiveTexture(GL_TEXTURE0);

    shader.setBool("alphaMask", flags.alphaMask);
    shader.setFloat("alphaCutoff", flags.alphaCutoff);
    shader.setMat4("model", parentTransform * localTransform);

    GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    if (flags.doubleSided)
        glDisable(GL_CULL_FACE);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    if (flags.doubleSided && cullFaceWasEnabled)
        glEnable(GL_CULL_FACE);
}
