#include "rendering/assets/mesh/SphereMesh.h"

SphereMesh::SphereMesh(unsigned int xSegments, unsigned int ySegments)
{
        constexpr float pi = 3.14159265359f;

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        vertices.reserve((xSegments + 1) * (ySegments + 1));
        indices.reserve(xSegments * ySegments * 6);

        for (unsigned int y = 0; y <= ySegments; ++y)
        {
            for (unsigned int x = 0; x <= xSegments; ++x)
            {
                float xSegment = static_cast<float>(x) / static_cast<float>(xSegments);
                float ySegment = static_cast<float>(y) / static_cast<float>(ySegments);
                float theta = xSegment * 2.0f * pi;
                float phi = ySegment * pi;

                glm::vec3 position(
                    std::sin(phi) * std::cos(theta),
                    std::cos(phi),
                    std::sin(phi) * std::sin(theta)
                );

                glm::vec3 tangent(-std::sin(theta), 0.0f, std::cos(theta));
                if (glm::length(tangent) < 0.0001f)
                {
                    tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                }

                Vertex vertex{};
                vertex.Position = position;
                vertex.Normal = glm::normalize(position);
                vertex.TexCoords = glm::vec2(xSegment, ySegment);
                vertex.Tangent = glm::normalize(tangent);
                vertex.Bitangent = glm::normalize(glm::cross(vertex.Normal, vertex.Tangent));
                vertices.push_back(vertex);
            }
        }

        for (unsigned int y = 0; y < ySegments; ++y)
        {
            for (unsigned int x = 0; x < xSegments; ++x)
            {
                unsigned int first = y * (xSegments + 1) + x;
                unsigned int second = first + xSegments + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        indexCount = static_cast<unsigned int>(indices.size());

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

        glBindVertexArray(0);
    }

void SphereMesh::draw() const
{
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

SphereMesh::~SphereMesh()
{
        glDeleteBuffers(1, &EBO);
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }
