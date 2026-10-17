#pragma once

#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/assets/texture/HDRTexture.h"
#include "rendering/uniforms/RenderParams.h"
#include "rendering/assets/light/LightSettings.h"
#include "scene/sceneObjectManager.h"
#include <functional>

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
};

class SceneRenderUI
{
public:
    void renderUI(SceneRenderUIState& uiState, 
        SceneObjectManager& manager,
        SceneObjectID selectedObjectID,
        float FPS, 
        float swapWaitMs
        );
};
