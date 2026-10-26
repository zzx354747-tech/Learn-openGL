#pragma once
#include <filesystem>
#include <fstream>
#include "scene/SceneDocument.h"

// Parsing has no side effects. RendererScene stages resources before applying this document.
inline SceneDocument loadScene(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open scene file: " + path.string());
    nlohmann::json json;
    file >> json;
    return sceneFromJson(json);
}
