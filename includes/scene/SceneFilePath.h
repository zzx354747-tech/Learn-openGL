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

// The startup environment is a preset, not an indication of which file was saved last.
inline std::filesystem::path latestSceneFilePath(
    const std::filesystem::path& directory = OPENGL_SCENE_SAVE_DIR)
{
    std::filesystem::path latest;
    std::filesystem::file_time_type newest{};
    for (const auto environment : {EnvironmentSelection::Night, EnvironmentSelection::Sunny,
                                    EnvironmentSelection::NightN8_3K})
    {
        const auto path = sceneFilePath(environment, directory);
        if (!std::filesystem::is_regular_file(path)) continue;
        const auto modified = std::filesystem::last_write_time(path);
        if (latest.empty() || modified > newest) { latest = path; newest = modified; }
    }
    if (latest.empty()) throw std::runtime_error("No saved scene found. Save a scene first.");
    return latest;
}
