#pragma once
#include "scene/SceneObject.h"
#include <algorithm>
#include <cmath>

namespace EditorTransform
{
// Scene Transform uses T * Rx * Ry * Rz * S. ImGuizmo's Euler helper uses a
// different convention, so extract XYZ explicitly rather than changing storage.
inline bool applyMatrix(const glm::mat4& matrix, Transform& transform)
{
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (!std::isfinite(matrix[c][r])) return false;
    glm::vec3 scale(glm::length(glm::vec3(matrix[0])),
                    glm::length(glm::vec3(matrix[1])),
                    glm::length(glm::vec3(matrix[2])));
    if (scale.x < 1e-6f || scale.y < 1e-6f || scale.z < 1e-6f) return false;
    if (glm::determinant(glm::mat3(matrix)) < 0) scale.x = -scale.x;
    const glm::mat3 rotation(glm::vec3(matrix[0]) / scale.x,
                             glm::vec3(matrix[1]) / scale.y,
                             glm::vec3(matrix[2]) / scale.z);
    if (std::abs(glm::dot(rotation[0], rotation[1])) > 0.001f ||
        std::abs(glm::dot(rotation[0], rotation[2])) > 0.001f ||
        std::abs(glm::dot(rotation[1], rotation[2])) > 0.001f) return false;
    const float sy = std::clamp(rotation[2][0], -1.0f, 1.0f);
    const float cy = std::hypot(rotation[0][0], rotation[1][0]);
    glm::vec3 angles;
    angles.y = std::atan2(sy, cy);
    if (cy > 1e-5f)
    {
        angles.x = std::atan2(-rotation[2][1], rotation[2][2]);
        angles.z = std::atan2(-rotation[1][0], rotation[0][0]);
    }
    else
    {
        angles.x = (sy >= 0 ? 1.0f : -1.0f) * std::atan2(rotation[0][1], rotation[1][1]);
        angles.z = 0;
    }
    transform.position = glm::vec3(matrix[3]);
    transform.rotationDegrees = glm::degrees(angles);
    transform.scale = scale;
    return true;
}
}
