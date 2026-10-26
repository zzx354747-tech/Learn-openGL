#pragma once

#include <nlohmann/json.hpp>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include "scene/SceneObjectManager.h"
#include "scene/Camera.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/uniforms/RenderParams.h"

// Persistent data contains no GL handles or owning pointers.
namespace glm
{
inline void to_json(nlohmann::json& j, const vec3& v)
{
    j = nlohmann::json::array({v.x, v.y, v.z});
}
inline void from_json(const nlohmann::json& j, vec3& v)
{
    if (!j.is_array() || j.size() != 3) throw std::runtime_error("Expected a vec3 array.");
    v = vec3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
    if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
        throw std::runtime_error("Non-finite vector.");
}
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneRenderConfig,
    enableSkybox, enableFlashlight, enablePointLight,
    enableDirectionalLight, enableGammaCorrection, enableHDR,
    enableBloom, enableSSAO, enablePBR,
    enableIBL, enableEdit, fixedAmbientStrength,
    iblAmbientStrength, phongDiffuseStrength, phongSpecularStrength,
    phongIBLDiffuseStrength, phongIBLSpecularStrength, ssaoStrength,
    exposure, bloomStrength, bloomThreshold,
    sunThreshold, directionalShadowLightSize, directionalShadowBlockerSearchRadius,
    directionalShadowMinFilterRadius, directionalShadowMaxFilterRadius, directionalShadowBiasSlope,
    directionalShadowBiasMin, fixedAmbientColor, iblAmbientTint,
    renderMode, forwardLightMode, sceneSelection,
    environmentSelection)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LightSettings,
    pointIntensity, pointAmbientIntensity, pointShadowStrength,
    pointConstant, pointLinear, pointQuadratic,
    sunIntensity, sunIntensityScale, sunShadowStrength,
    sunExtractedFromEnvironment, flashIntensity, flashShadowStrength,
    flashConstant, flashLinear, flashQuadratic,
    flashCutOff, flashOuterCutOff, flashRightOffset,
    flashUpOffset, flashForwardOffset, pointAmbient,
    pointDiffuse, pointSpecular, sunDirection,
    sunAmbient, sunDiffuse, sunSpecular,
    flashAmbient, flashDiffuse, flashSpecular)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RenderParams,
    enableNormalMapping, enableParallaxMapping, bumpNormalStrength,
    numLayers, parallaxHeightScale)

struct SceneCameraData
{
    glm::vec3 position{0, 0, 3};
    glm::vec3 worldUp{0, 1, 0};
    float yaw = -90, pitch = 0, movementSpeed = 2.5f, mouseSensitivity = 0.1f;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneCameraData, position, worldUp, yaw, pitch,
                                 movementSpeed, mouseSensitivity)

struct SceneEditorData
{
    SceneObjectID selectedObjectID = InvalidSceneObjectID;
    int gizmoOperation = 0, gizmoSpace = 0;
    bool gizmoSnap = false;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SceneEditorData, selectedObjectID, gizmoOperation, gizmoSpace, gizmoSnap)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transform, position, rotationDegrees, scale)

// Keep the existing flat transform keys in scene.json.
inline void to_json(nlohmann::json& j, const SceneObjectData& o)
{
    j = {{"id", o.id}, {"name", o.name}, {"position", o.transform.position},
         {"rotationDegrees", o.transform.rotationDegrees}, {"scale", o.transform.scale},
         {"scene", o.scene}, {"isVisible", o.isVisible}, {"type", o.type},
         {"modelPath", o.modelPath}, {"materialIndex", o.materialIndex}};
}
inline void from_json(const nlohmann::json& j, SceneObjectData& o)
{
    j.at("id").get_to(o.id);
    j.at("name").get_to(o.name);
    j.get_to(o.transform);
    j.at("scene").get_to(o.scene);
    j.at("isVisible").get_to(o.isVisible);
    // Old saves can identify built-in objects by their stable IDs.
    o.type = j.value("type", o.id <= MaterialSphereCount ? "sphere" : "livingRoom");
    o.modelPath = j.value("modelPath", std::string{});
    o.materialIndex = j.value("materialIndex", o.id > 0 ? o.id - 1 : 0);
    if (!j.contains("type") && o.id > MaterialSphereCount + 1)
        throw std::runtime_error("Old save lacks imported model sources; save again with the current version.");
}

struct SceneDocument
{
    SceneRenderConfig config;
    LightSettings lighting;
    RenderParams surface;
    SceneRenderState state;
    SceneCameraData camera;
    SceneEditorData editor;
    std::vector<SceneObjectData> objects;
};

inline nlohmann::json sceneToJson(const SceneDocument& d)
{
    auto positions = nlohmann::json::array();
    for (const auto& p : d.state.materialSpherePositions) positions.push_back(p);
    return {{"formatVersion", 2}, {"objects", d.objects}, {"config", d.config},
        {"lighting", d.lighting}, {"surface", d.surface}, {"camera", d.camera},
        {"editor", d.editor}, {"state", {{"lightPositions", d.state.lightPositions},
        {"color", d.state.color}, {"materialSpherePositions", positions}}}};
}

inline void validateScene(const SceneDocument& d)
{
    auto range = [](int v, int max) { if (v < 0 || v > max) throw std::runtime_error("Invalid enum value."); };
    range(static_cast<int>(d.config.renderMode), 3);
    range(static_cast<int>(d.config.forwardLightMode), 2);
    range(static_cast<int>(d.config.sceneSelection), 2);
    range(static_cast<int>(d.config.environmentSelection), 2);
    range(d.editor.gizmoOperation, 2);
    range(d.editor.gizmoSpace, 1);
    if (glm::length(d.camera.worldUp) < 0.0001f || std::abs(d.camera.pitch) >= 90 ||
        d.camera.movementSpeed < 0 || d.camera.mouseSensitivity < 0 || d.surface.numLayers < 1)
        throw std::runtime_error("Invalid camera or surface parameters.");
    const auto front = glm::vec3(std::cos(glm::radians(d.camera.yaw)) * std::cos(glm::radians(d.camera.pitch)),
        std::sin(glm::radians(d.camera.pitch)), std::sin(glm::radians(d.camera.yaw)) * std::cos(glm::radians(d.camera.pitch)));
    if (glm::length(glm::cross(front, glm::normalize(d.camera.worldUp))) < 0.0001f)
        throw std::runtime_error("Camera up is parallel to view direction.");
    std::set<SceneObjectID> ids;
    for (const auto& o : d.objects)
    {
        range(static_cast<int>(o.scene), 2);
        if (o.id == 0 || o.id == std::numeric_limits<SceneObjectID>::max() || !ids.insert(o.id).second)
            throw std::runtime_error("Invalid or duplicate object ID.");
        if (o.type != "sphere" && o.type != "livingRoom" && o.type != "model")
            throw std::runtime_error("Unknown object type.");
        if (o.type == "sphere" && o.materialIndex >= MaterialSphereCount)
            throw std::runtime_error("Invalid sphere material index.");
        if (o.type == "model" && o.modelPath.empty()) throw std::runtime_error("Missing model source.");
        if (o.transform.scale.x == 0 || o.transform.scale.y == 0 || o.transform.scale.z == 0)
            throw std::runtime_error("Object scale must be nonzero.");
    }
    if (d.editor.selectedObjectID != 0 && ids.count(d.editor.selectedObjectID) == 0)
        throw std::runtime_error("Selected object does not exist.");
}

inline SceneDocument sceneFromJson(const nlohmann::json& j)
{
    auto integer = [](const nlohmann::json& v, double min, double max) {
        if (!v.is_number_integer() || v.get<double>() < min || v.get<double>() > max)
            throw std::runtime_error("Invalid integer scene field.");
    };
    if (j.contains("formatVersion")) integer(j.at("formatVersion"), 1, 2);
    for (const auto* field : {"renderMode", "forwardLightMode", "sceneSelection", "environmentSelection"})
        integer(j.at("config").at(field), 0, 3);
    integer(j.at("surface").at("numLayers"), 1, std::numeric_limits<int>::max());
    for (const auto& object : j.at("objects"))
    {
        integer(object.at("id"), 1, std::numeric_limits<SceneObjectID>::max() - 1);
        integer(object.at("scene"), 0, 2);
        if (object.contains("materialIndex")) integer(object.at("materialIndex"), 0, std::numeric_limits<unsigned>::max());
    }
    if (j.contains("editor"))
    {
        integer(j.at("editor").at("selectedObjectID"), 0, std::numeric_limits<SceneObjectID>::max() - 1);
        integer(j.at("editor").at("gizmoOperation"), 0, 2);
        integer(j.at("editor").at("gizmoSpace"), 0, 1);
    }
    if (j.value("formatVersion", 1) < 1 || j.value("formatVersion", 1) > 2)
        throw std::runtime_error("Unsupported scene format version.");
    // Reject null/NaN/infinite numbers before conversion (including overflowing floats).
    auto checkNumbers = [](const auto& self, const nlohmann::json& v) -> void {
        if (v.is_null()) throw std::runtime_error("Null scene value.");
        if (v.is_number() && (!std::isfinite(v.get<double>()) || std::abs(v.get<double>()) > std::numeric_limits<float>::max()))
            throw std::runtime_error("Non-finite scene number.");
        if (v.is_structured()) for (const auto& child : v) self(self, child);
    };
    checkNumbers(checkNumbers, j);
    SceneDocument d;
    j.at("config").get_to(d.config);
    j.at("lighting").get_to(d.lighting);
    j.at("surface").get_to(d.surface);
    j.at("camera").get_to(d.camera);
    j.at("objects").get_to(d.objects);
    const auto& state = j.at("state");
    state.at("lightPositions").get_to(d.state.lightPositions);
    state.at("color").get_to(d.state.color);
    if (state.contains("materialSpherePositions"))
    {
        const auto& p = state.at("materialSpherePositions");
        if (!p.is_array() || p.size() != MaterialSphereCount) throw std::runtime_error("Invalid sphere positions.");
        for (unsigned i = 0; i < MaterialSphereCount; ++i) p.at(i).get_to(d.state.materialSpherePositions[i]);
    }
    if (j.contains("editor")) j.at("editor").get_to(d.editor);
    validateScene(d);
    return d;
}
