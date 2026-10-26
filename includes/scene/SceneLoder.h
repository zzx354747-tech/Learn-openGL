#pragma once
#include <fstream>
#include <iostream>
#include <exception>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include "Camera.h"

inline bool loadScene(Camera& camera)
{
    std::ifstream file(OPENGL_SCENE_SAVE_FILE);
    if (!file.is_open())
    {
        std::cerr << "Failed to open scene file for reading: " << OPENGL_SCENE_SAVE_FILE << std::endl;
        return false;
    }

    // 创建json对象
    nlohmann::json j;
    try
    {
        file >> j;

        // 读取场景配置
        auto position = j.at("camera").at("position");
        float posX = position.at(0).get<float>();
        float posY = position.at(1).get<float>();
        float posZ = position.at(2).get<float>();

        float yaw = j.at("camera").at("yaw").get<float>();
        float pitch = j.at("camera").at("pitch").get<float>();

        auto up = j.at("camera").at("worldUp");

        float upX = up.at(0).get<float>();
        float upY = up.at(1).get<float>();
        float upZ = up.at(2).get<float>();

glm::vec3 cameraWorldUp(upX, upY, upZ);

        glm::vec3 cameraPosition(posX, posY, posZ);

        float movementSpeed = j.at("camera").at("movementSpeed").get<float>();
        float mouseSensitivity = j.at("camera").at("mouseSensitivity").get<float>();

        camera.setCameraState
(
            cameraPosition,
            cameraWorldUp,
            yaw,
            pitch,
            movementSpeed,
            mouseSensitivity
        );


        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing scene file: " << e.what() << std::endl;
        return false;
    }
}
