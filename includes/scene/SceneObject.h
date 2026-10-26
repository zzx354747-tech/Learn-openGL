#pragma once

#include <cstdint>
#include <variant>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rendering/resources/render/SceneRenderTypes.h"

class ModelDrawer;
class SphereDrawer;

using SceneObjectID = std::uint32_t;
// 编译器常量，表示无效的场景对象ID
constexpr SceneObjectID InvalidSceneObjectID = 0;

struct Transform
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotationDegrees{0.0f, 0.0f, 0.0f}; 
    glm::vec3 scale{1.0f, 1.0f, 1.0f};

    glm::mat4 matrix() const
    {
        glm::mat4 result = glm::mat4(1.0f);
        result = glm::translate(result, position);
        result = glm::rotate(result, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
        result = glm::rotate(result, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
        result = glm::rotate(result, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
        result = glm::scale(result, scale);
        return result;
    }
};

struct sphereRenderRef
{
    SphereDrawer* drawer = nullptr;
    unsigned int materialIndex = 0;
};

using SceneObjectRef = std::variant<ModelDrawer*, sphereRenderRef>;

struct SceneObject
{
    SceneObjectID id = InvalidSceneObjectID;
    std::string name;

    Transform transform;
    SceneObjectRef ref;

    SceneSelection scene = SceneSelection::Default;

    bool isVisible = true;
};
