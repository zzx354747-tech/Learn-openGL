#pragma once
#include <filesystem>
#include <stdexcept>
#include "rendering/resources/render/SceneRenderTypes.h"

inline std::filesystem::path sceneFilePath(
    EnvironmentSelection environment,
    const std::filesystem::path& directory = OPENGL_SCENE_SAVE_DIR)
{
    switch (environment)
    {
    case EnvironmentSelection::Night: return directory / "night.json";
    case EnvironmentSelection::Sunny: return directory / "sunny.json";
    case EnvironmentSelection::NightN8_3K: return directory / "night_n8_3k.json";
    }
    throw std::runtime_error("Unknown environment for scene save.");
}
