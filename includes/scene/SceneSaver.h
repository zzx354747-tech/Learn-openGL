#pragma once

#include <fstream>
#include <filesystem>
#include <system_error>
#include <nlohmann/json.hpp>

#include "scene/SceneObjectManager.h"
#include "scene/Camera.h"
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/uniforms/RenderParams.h"

// 将 glm::vec3 转成 JSON 数组，例如 [1.0, 2.0, 3.0]。
// 放进命名空间，避免辅助函数名称和其他代码冲突。
namespace SceneSaverDetail
{
    inline nlohmann::json vec3ToJson(const glm::vec3& value)
    {
        return nlohmann::json::array({
            value.x,
            value.y,
            value.z
        });
    }
}

inline bool saveScene(
    const SceneObjectManager& manager,
    const LightSettings& lightSettings,
    const SceneRenderConfig& config,
    const RenderParams& params,
    const SceneRenderState& state,
    const Camera& camera)
{
    using json = nlohmann::json;
    using SceneSaverDetail::vec3ToJson;

    json j = json::object();

    // ==================== 物体 ====================

    j["objects"] = json::array();

    for (const auto& object : manager.getSceneObjects())
    {
        json objJson;

        objJson["id"] = object.id;
        objJson["name"] = object.name;

        objJson["position"] =
            vec3ToJson(object.transform.position);

        objJson["rotationDegrees"] =
            vec3ToJson(object.transform.rotationDegrees);

        objJson["scale"] =
            vec3ToJson(object.transform.scale);

        objJson["scene"] = static_cast<int>(object.scene);
        objJson["isVisible"] = object.isVisible;

        j["objects"].push_back(objJson);
    }

    // ==================== 场景配置 ====================

    auto& c = j["config"];

    // 功能开关
    c["enableSkybox"] = config.enableSkybox;
    c["enableFlashlight"] = config.enableFlashlight;
    c["enablePointLight"] = config.enablePointLight;
    c["enableDirectionalLight"] = config.enableDirectionalLight;
    c["enableGammaCorrection"] = config.enableGammaCorrection;
    c["enableHDR"] = config.enableHDR;
    c["enableBloom"] = config.enableBloom;
    c["enableSSAO"] = config.enableSSAO;
    c["enablePBR"] = config.enablePBR;
    c["enableIBL"] = config.enableIBL;
    c["enableEdit"] = config.enableEdit;

    // 环境光
    c["fixedAmbientColor"] =
        vec3ToJson(config.fixedAmbientColor);

    c["fixedAmbientStrength"] =
        config.fixedAmbientStrength;

    c["iblAmbientTint"] =
        vec3ToJson(config.iblAmbientTint);

    c["iblAmbientStrength"] =
        config.iblAmbientStrength;

    // Phong 参数
    c["phongDiffuseStrength"] =
        config.phongDiffuseStrength;

    c["phongSpecularStrength"] =
        config.phongSpecularStrength;

    c["phongIBLDiffuseStrength"] =
        config.phongIBLDiffuseStrength;

    c["phongIBLSpecularStrength"] =
        config.phongIBLSpecularStrength;

    // 后处理
    c["ssaoStrength"] = config.ssaoStrength;
    c["exposure"] = config.exposure;
    c["bloomStrength"] = config.bloomStrength;
    c["bloomThreshold"] = config.bloomThreshold;

    // 环境太阳提取阈值
    c["sunThreshold"] = config.sunThreshold;

    // 方向光阴影
    c["directionalShadowLightSize"] =
        config.directionalShadowLightSize;

    c["directionalShadowBlockerSearchRadius"] =
        config.directionalShadowBlockerSearchRadius;

    c["directionalShadowMinFilterRadius"] =
        config.directionalShadowMinFilterRadius;

    c["directionalShadowMaxFilterRadius"] =
        config.directionalShadowMaxFilterRadius;

    c["directionalShadowBiasSlope"] =
        config.directionalShadowBiasSlope;

    c["directionalShadowBiasMin"] =
        config.directionalShadowBiasMin;

    // 模式与场景选择
    c["renderMode"] =
        static_cast<int>(config.renderMode);

    c["forwardLightMode"] =
        static_cast<int>(config.forwardLightMode);

    c["sceneSelection"] =
        static_cast<int>(config.sceneSelection);

    c["environmentSelection"] =
        static_cast<int>(config.environmentSelection);

    // ==================== 灯光 ====================

    auto& l = j["lighting"];

    // 点光源
    l["pointAmbient"] =
        vec3ToJson(lightSettings.pointAmbient);

    l["pointDiffuse"] =
        vec3ToJson(lightSettings.pointDiffuse);

    l["pointSpecular"] =
        vec3ToJson(lightSettings.pointSpecular);

    l["pointIntensity"] =
        lightSettings.pointIntensity;

    l["pointAmbientIntensity"] =
        lightSettings.pointAmbientIntensity;

    l["pointShadowStrength"] =
        lightSettings.pointShadowStrength;

    l["pointConstant"] = lightSettings.pointConstant;
    l["pointLinear"] = lightSettings.pointLinear;
    l["pointQuadratic"] = lightSettings.pointQuadratic;

    // 方向光
    l["sunDirection"] =
        vec3ToJson(lightSettings.sunDirection);

    l["sunAmbient"] =
        vec3ToJson(lightSettings.sunAmbient);

    l["sunDiffuse"] =
        vec3ToJson(lightSettings.sunDiffuse);

    l["sunSpecular"] =
        vec3ToJson(lightSettings.sunSpecular);

    l["sunIntensity"] =
        lightSettings.sunIntensity;

    l["sunIntensityScale"] =
        lightSettings.sunIntensityScale;

    l["sunShadowStrength"] =
        lightSettings.sunShadowStrength;

    l["sunExtractedFromEnvironment"] =
        lightSettings.sunExtractedFromEnvironment;

    // 手电筒
    l["flashAmbient"] =
        vec3ToJson(lightSettings.flashAmbient);

    l["flashDiffuse"] =
        vec3ToJson(lightSettings.flashDiffuse);

    l["flashSpecular"] =
        vec3ToJson(lightSettings.flashSpecular);

    l["flashIntensity"] =
        lightSettings.flashIntensity;

    l["flashShadowStrength"] =
        lightSettings.flashShadowStrength;

    l["flashConstant"] = lightSettings.flashConstant;
    l["flashLinear"] = lightSettings.flashLinear;
    l["flashQuadratic"] = lightSettings.flashQuadratic;

    l["flashCutOff"] = lightSettings.flashCutOff;
    l["flashOuterCutOff"] = lightSettings.flashOuterCutOff;

    l["flashRightOffset"] = lightSettings.flashRightOffset;
    l["flashUpOffset"] = lightSettings.flashUpOffset;
    l["flashForwardOffset"] = lightSettings.flashForwardOffset;

    // ==================== 表面效果 ====================

    auto& p = j["surface"];

    p["enableNormalMapping"] =
        params.enableNormalMapping;

    p["enableParallaxMapping"] =
        params.enableParallaxMapping;

    p["bumpNormalStrength"] =
        params.bumpNormalStrength;

    p["numLayers"] =
        params.numLayers;

    p["parallaxHeightScale"] =
        params.parallaxHeightScale;

    // ==================== 场景状态 ====================

    auto& s = j["state"];

    s["lightPositions"] = vec3ToJson(state.lightPositions);
    s["color"] = vec3ToJson(state.color);

    // 球体的实际位置已经保存在 objects 中。
    // 阴影矩阵在运行时重新计算，因此这里不保存。

    // ==================== 相机 ====================

    auto& cam = j["camera"];

    cam["position"] = vec3ToJson(camera.Getposition());
    cam["worldUp"] = vec3ToJson(camera.GetWorldUp());
    cam["yaw"] = camera.GetYaw();
    cam["pitch"] = camera.GetPitch();
    cam["movementSpeed"] = camera.GetMovementSpeed();
    cam["mouseSensitivity"] = camera.GetMouseSensitivity();

    // Front、Up、Right 可以根据朝向重新计算。
    // 鼠标历史位置、首次输入标志和移动速度向量不保存。

    // ==================== 写入文件 ====================

    const std::filesystem::path outputFile(OPENGL_SCENE_SAVE_FILE);
    const std::filesystem::path outputDirectory = outputFile.parent_path();
    std::error_code directoryError;
    std::filesystem::create_directories(outputDirectory, directoryError);
    if (directoryError)
    {
        return false;
    }

    std::ofstream file(outputFile);

    if (!file.is_open())
    {
        return false;
    }

    // 缩进 4 个空格，方便查看生成的文件。
    file << j.dump(4);

    file.close();

    return file.good();
}
