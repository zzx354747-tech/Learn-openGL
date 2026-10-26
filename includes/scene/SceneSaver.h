#pragma once
#include <filesystem>
#include <fstream>
#include "scene/SceneDocument.h"
#include "scene/SceneFilePath.h"

// Replace the previous save only after a complete, checked write.
inline bool saveScene(const SceneDocument& document,
                      const std::filesystem::path& requestedPath = {})
{
    const auto path = requestedPath.empty() ? sceneFilePath(document.config.environmentSelection) : requestedPath;
    validateScene(document);
    const auto json = sceneToJson(document);
    // Validate numeric values as well, before touching the previous save.
    sceneFromJson(json);
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
    auto temporary = path;
    temporary += ".tmp";
    try
    {
        std::ofstream file(temporary, std::ios::trunc);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << json.dump(4) << '\n';
        file.close();
        std::filesystem::rename(temporary, path);
        return true;
    }
    catch (...)
    {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}
