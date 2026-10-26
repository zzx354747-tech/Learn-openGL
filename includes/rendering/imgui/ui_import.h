#pragma once

#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/assets/texture/HDRTexture.h"
#include "rendering/uniforms/RenderParams.h"
#include "rendering/assets/light/LightSettings.h"
#include "scene/SceneObjectManager.h"
#include <functional>
#include <string>
#include "scene/Camera.h"

struct SceneRenderUIState
{
    int renderModeIndex = 0;
    int environmentIndex = 0;

    SceneRenderConfig& sceneConfig;
    RenderParams& renderParams;
    LightSettings& lightSettings;

    bool environmentLoadFailed = false;

    std::function<bool()> loadEnvironment;
    std::function<void()> applyEnvironmentPreset;
    std::function<void(const std::string&)> queueModelImport;
    std::string importMessage;
    bool importFailed = false;
    std::function<bool()> saveSceneToFile;
    std::function<bool()> loadSceneFromFile;
    std::string sceneFileMessage;
    bool sceneFileFailed = false;
};

class SceneRenderUI
{
public:
    void renderUI(SceneRenderUIState& uiState, 
        SceneObjectManager& manager,
        SceneObjectID& selectedObjectID,
        float FPS, 
        float swapWaitMs,
        unsigned int viewportTexture,
        bool cameraCaptured,
        Camera& camera
        );

    static void configureStyle();
    bool acceptsModelDrop(float x, float y) const;
    bool gizmoHovered = false;
    bool gizmoUsing = false;
    int gizmoOperation = 0; // Translate, rotate, scale.
    int gizmoSpace = 0;     // World, local.
    bool gizmoSnap = false;

    // Logical ImGui coordinates; convert to framebuffer pixels at the input boundary.
    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportWidth = 1.0f;
    float viewportHeight = 1.0f;
    bool viewportClicked = false;
    bool viewportVisible = false;

private:
    char objectSearch[128] = {};
    char modelPath[1024] = {};
    float importX = 0, importY = 0, importWidth = 0, importHeight = 0;
    SceneObjectID inspectedObject = InvalidSceneObjectID;
    bool showScenePanel = true;
    bool showInspector = true;
    float scenePanelWidth = 230.0f;
    float inspectorWidth = 350.0f;
};
