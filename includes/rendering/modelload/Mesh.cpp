#include "Mesh.h"

namespace
{
unsigned int getDefaultWhiteTexture()
{
    static unsigned int textureID = 0;

    if (textureID == 0)
    {
        unsigned char whitePixel[] = {255, 255, 255, 255};

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            1,
            1,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            whitePixel
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return textureID;
}

unsigned int getDefaultBlackTexture()
{
    static unsigned int textureID = 0;

    if (textureID == 0)
    {
        unsigned char blackPixel[] = {0, 0, 0, 255};

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            1,
            1,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            blackPixel
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    return textureID;
}
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, position));
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, normal));
    // texCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texCoords));

    // texCoords1
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texCoords1));

    // tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(vertex),
        (void*)offsetof(vertex, tangent)
    );

    // bitangent
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(vertex),
        (void*)offsetof(vertex, bitangent)
    );

    glBindVertexArray(0);
}

void Mesh::draw(Shader& shader)
{
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    unsigned int roughnessNr = 1;
    unsigned int metallicNr = 1;
    unsigned int metallicRoughnessNr = 1;
    bool hasNormalMap = false;
    bool hasParallaxMap = false;
    bool hasRoughnessMap = false;
    bool hasMetallicMap = false;
    unsigned int textureUnit = 2;

    shader.setBool("hasSpecularMap", false);
    shader.setBool("usePackedMetallicRoughness", false);
    shader.setBool("hasRoughnessMap", false);
    shader.setBool("hasMetallicMap", false);
    shader.setVec4("baseColorFactor", materialFactors.baseColor);
    shader.setFloat("roughnessFactor", materialFactors.roughness);
    shader.setFloat("metallicFactor", materialFactors.metallic);
    shader.setBool("alphaMask", materialFactors.alphaMask);
    shader.setFloat("alphaCutoff", materialFactors.alphaCutoff);
    shader.setInt("albedoTexCoordIndex", 0);
    shader.setInt("normalTexCoordIndex", 0);
    shader.setInt("parallaxTexCoordIndex", 0);
    shader.setInt("roughnessTexCoordIndex", 0);
    shader.setInt("metallicTexCoordIndex", 0);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("albedoTexture", 0);
    shader.setInt("roughnessTexture", 0);
    shader.setInt("metallicTexture", 1);
    shader.setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getDefaultWhiteTexture());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, getDefaultBlackTexture());

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);

        std::string number;
        std::string name = textures[i].type;

        if (name == "texture_diffuse" || name == "texture_basecolor")
        {
            number = std::to_string(diffuseNr++);
            if (number == "1")
            {
                shader.setInt("diffuseTexture", textureUnit);
                shader.setInt("albedoTexture", textureUnit);
                shader.setInt("albedoTexCoordIndex", textures[i].uvIndex);
            }
        }
        else if (name == "texture_specular")
        {
            number = std::to_string(specularNr++);
            if (number == "1")
            {
                shader.setInt("specularTexture", textureUnit);
                shader.setBool("hasSpecularMap", true);
            }
        }
        else if (name == "texture_normal")
        {
            number = std::to_string(normalNr++);
            hasNormalMap = true;
            if (number == "1")
            {
                shader.setInt("normalTexture", textureUnit);
                shader.setInt("normalTexCoordIndex", textures[i].uvIndex);
            }
        }
        else if (name == "texture_height")
        {
            number = std::to_string(heightNr++);
            hasParallaxMap = true;
            if (number == "1")
            {
                shader.setInt("parallaxTexture", textureUnit);
                shader.setInt("parallaxTexCoordIndex", textures[i].uvIndex);
            }
        }
        else if (name == "texture_roughness")
        {
            number = std::to_string(roughnessNr++);
            hasRoughnessMap = true;
            if (number == "1")
            {
                shader.setInt("roughnessTexture", textureUnit);
                shader.setInt("roughnessTexCoordIndex", textures[i].uvIndex);
            }
        }
        else if (name == "texture_metallic")
        {
            number = std::to_string(metallicNr++);
            hasMetallicMap = true;
            if (number == "1")
            {
                shader.setInt("metallicTexture", textureUnit);
                shader.setInt("metallicTexCoordIndex", textures[i].uvIndex);
            }
        }
        else if (name == "texture_metallic_roughness")
        {
            number = std::to_string(metallicRoughnessNr++);
            hasRoughnessMap = true;
            hasMetallicMap = true;
            if (number == "1")
            {
                shader.setInt("metallicTexture", textureUnit);
                shader.setInt("roughnessTexture", textureUnit);
                shader.setInt("metallicTexCoordIndex", textures[i].uvIndex);
                shader.setInt("roughnessTexCoordIndex", textures[i].uvIndex);
                shader.setBool("usePackedMetallicRoughness", true);
            }
        }

        shader.setInt(name + number, textureUnit);

        glBindTexture(GL_TEXTURE_2D, textures[i].id);
        textureUnit++;
    }

    shader.setBool("hasNormalMap", hasNormalMap);
    shader.setBool("hasParallaxMap", hasParallaxMap);
    shader.setBool("hasRoughnessMap", hasRoughnessMap);
    shader.setBool("hasMetallicMap", hasMetallicMap);

    glBindVertexArray(VAO);

    glDrawElements(
        GL_TRIANGLES,
        static_cast<unsigned int>(indices.size()),
        GL_UNSIGNED_INT,
        0
    );

    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
}
