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
    bool hasNormalMap = false;
    bool hasParallaxMap = false;
    unsigned int textureUnit = 1;

    shader.setBool("hasSpecularMap", false);
    shader.setInt("diffuseTexture", 0);
    shader.setInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, getDefaultWhiteTexture());

    for (unsigned int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);

        std::string number;
        std::string name = textures[i].type;

        if (name == "texture_diffuse")
        {
            number = std::to_string(diffuseNr++);
            if (number == "1")
            {
                shader.setInt("diffuseTexture", textureUnit);
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
            }
        }
        else if (name == "texture_height")
        {
            number = std::to_string(heightNr++);
            hasParallaxMap = true;
            if (number == "1")
            {
                shader.setInt("parallaxTexture", textureUnit);
            }
        }

        shader.setInt(name + number, textureUnit);

        glBindTexture(GL_TEXTURE_2D, textures[i].id);
        textureUnit++;
    }

    shader.setBool("hasNormalMap", hasNormalMap);
    shader.setBool("hasParallaxMap", hasParallaxMap);

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
