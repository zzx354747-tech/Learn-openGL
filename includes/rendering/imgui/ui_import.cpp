#include "ui_import.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include "rendering/resources/environment/EnvironmentOption.h"

void SceneRenderUI::renderUI(
    SceneRenderUIState& uiState,
    float FPS,
    float swapWaitMs)
{
     ImGui::Begin("Deferred PBR Renderer");
        ImGui::Text("FPS: %.2f", FPS);
        ImGui::Text("Swap wait ms: %.3f", swapWaitMs);
        if (uiState.environmentLoadFailed)
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                "Environment load failed! Check console.");
        }

        ImGui::SeparatorText("Render");
        const char* sceneNames[] = {"Default", "Living Room", "Fuji Terrain"};
        int sceneIndex = 0;
        if (uiState.sceneConfig.sceneSelection == SceneSelection::LivingRoom)
            sceneIndex = 1;
        else if (uiState.sceneConfig.sceneSelection == SceneSelection::FujiTerrain)
            sceneIndex = 2;
        if (ImGui::Combo("Scene", &sceneIndex, sceneNames, 3))
        {
            uiState.sceneConfig.sceneSelection = sceneIndex == 1
                ? SceneSelection::LivingRoom
                : (sceneIndex == 2 ? SceneSelection::FujiTerrain : SceneSelection::Default);
        }

        const char* renderModeNames[] = {"Lighting", "Forward Basic", "Forward Reflection", "Shadow Debug"};

        switch (uiState.sceneConfig.renderMode)
        {
            case RenderMode::Basic:
                uiState.renderModeIndex = 1;
                break;
            case RenderMode::Reflection:
                uiState.renderModeIndex = 2;
                break;
            case RenderMode::ShadowDebug:
                uiState.renderModeIndex = 3;
                break;
            case RenderMode::Lighting:
            default:
                uiState.renderModeIndex = 0;
                break;
        }

        if (ImGui::Combo("Render Mode", &uiState.renderModeIndex, renderModeNames, 4))
        {
            switch (uiState.renderModeIndex)
            {
                case 1:
                    uiState.sceneConfig.renderMode = RenderMode::Basic;
                    uiState.sceneConfig.forwardLightMode = ForwardLightMode::Basic;
                    break;
                case 2:
                    uiState.sceneConfig.renderMode = RenderMode::Reflection;
                    uiState.sceneConfig.forwardLightMode = ForwardLightMode::Reflect;
                    break;
                case 3:
                    uiState.sceneConfig.renderMode = RenderMode::ShadowDebug;
                    uiState.sceneConfig.forwardLightMode = ForwardLightMode::Light;
                    break;
                default:
                    uiState.sceneConfig.renderMode = RenderMode::Lighting;
                    uiState.sceneConfig.forwardLightMode = ForwardLightMode::Light;
                    break;
            }
        }
        const char* environmentNames[] = {"Night", "Sunny", "God Rays 07 3K", "Night N8 3K"};
        if (ImGui::Combo("Environment", &uiState.environmentIndex, environmentNames, 4))
        {
            uiState.sceneConfig.environmentSelection = kEnvironmentOptions[uiState.environmentIndex].selection;
            uiState.environmentLoadFailed = !uiState.loadEnvironment();
            if (!uiState.environmentLoadFailed)
            {
                uiState.applyEnvironmentPreset();
                uiState.renderModeIndex = 0;
            }
        }
        ImGui::Checkbox("Skybox", &uiState.sceneConfig.enableSkybox);
        ImGui::Checkbox("Gamma Correction", &uiState.sceneConfig.enableGammaCorrection);
        ImGui::Checkbox("HDR", &uiState.sceneConfig.enableHDR);
        ImGui::Checkbox("PBR", &uiState.sceneConfig.enablePBR);
        ImGui::Checkbox("IBL", &uiState.sceneConfig.enableIBL);
        ImGui::Checkbox("Animated Water", &uiState.sceneConfig.enableWater);

        ImGui::SeparatorText("Ambient");
        ImGui::ColorEdit3("Fixed Ambient Color", glm::value_ptr(uiState.sceneConfig.fixedAmbientColor));
        ImGui::DragFloat("Fixed Ambient Strength", &uiState.sceneConfig.fixedAmbientStrength, 0.01f, 0.0f, 4.0f);
        ImGui::ColorEdit3("IBL Ambient Tint", glm::value_ptr(uiState.sceneConfig.iblAmbientTint));
        ImGui::DragFloat("IBL Ambient Strength", &uiState.sceneConfig.iblAmbientStrength, 0.01f, 0.0f, 4.0f);

        ImGui::SeparatorText("Phong Settings");
        ImGui::DragFloat("Phong Diffuse Strength", &uiState.sceneConfig.phongDiffuseStrength, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Phong Specular Strength", &uiState.sceneConfig.phongSpecularStrength, 0.01f, 0.0f, 2.0f);
        ImGui::DragFloat("Phong IBL Diffuse", &uiState.sceneConfig.phongIBLDiffuseStrength, 0.01f, 0.0f, 6.0f);
        ImGui::DragFloat("Phong IBL Specular", &uiState.sceneConfig.phongIBLSpecularStrength, 0.01f, 0.0f, 4.0f);

        ImGui::SeparatorText("Global Illumination");
        ImGui::Checkbox("Screen-space GI", &uiState.sceneConfig.enableGI);
        ImGui::SliderFloat("GI Strength", &uiState.sceneConfig.giStrength, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("GI Radius", &uiState.sceneConfig.giRadius, 2.0f, 40.0f, "%.1f px");
        ImGui::SliderFloat("GI Max Distance", &uiState.sceneConfig.giMaxDistance, 1.0f, 20.0f, "%.1f");
        ImGui::SliderInt("GI Samples", &uiState.sceneConfig.giSampleCount, 4, 16);

        ImGui::SeparatorText("Material Mapping");
        ImGui::Checkbox("Normal Mapping", &uiState.renderParams.enableNormalMapping);
        ImGui::Checkbox("Parallax Mapping", &uiState.renderParams.enableParallaxMapping);
        ImGui::SliderFloat("Parallax Height Scale", &uiState.renderParams.parallaxHeightScale, 0.0f, 0.1f, "%.3f");
        ImGui::SliderFloat("Bump Normal Strength", &uiState.renderParams.bumpNormalStrength, 0.0f, 10.0f, "%.2f");
        ImGui::SliderInt("Parallax Layers", &uiState.renderParams.numLayers, 1, 64);

        ImGui::SeparatorText("Post Process");
        ImGui::Checkbox("SSAO", &uiState.sceneConfig.enableSSAO);
        ImGui::SliderFloat("SSAO Strength", &uiState.sceneConfig.ssaoStrength, 0.0f, 4.0f, "%.2f");
        ImGui::Checkbox("Bloom", &uiState.sceneConfig.enableBloom);
        ImGui::SliderFloat("Exposure", &uiState.sceneConfig.exposure, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Bloom Strength", &uiState.sceneConfig.bloomStrength, 0.0f, 3.0f, "%.2f");
        ImGui::SliderFloat("Bloom Threshold", &uiState.sceneConfig.bloomThreshold, 0.0f, 3.0f, "%.2f");
        ImGui::SliderInt("Number of Blur Passes", &uiState.sceneConfig.numBlurPasses, 1, 20);

        ImGui::SeparatorText("Lights");
        ImGui::Checkbox("Point Light", &uiState.sceneConfig.enablePointLight);
        ImGui::Checkbox("Directional Light", &uiState.sceneConfig.enableDirectionalLight);
        ImGui::Checkbox("Flashlight", &uiState.sceneConfig.enableFlashlight);
        ImGui::DragFloat("Sun Threshold", &uiState.sceneConfig.sunThreshold, 1.0f, 0.0f, 1000.0f, "%.1f");
        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            // 按照代码顺序进行检测
            uiState.environmentLoadFailed = !uiState.loadEnvironment();
        }

        if (uiState.sceneConfig.enablePointLight)
        {
            ImGui::SeparatorText("Point Light Settings");

            ImGui::ColorEdit3("Point Light Ambient", glm::value_ptr(uiState.lightSettings.pointAmbient));
            ImGui::ColorEdit3("Point Light Diffuse", glm::value_ptr(uiState.lightSettings.pointDiffuse));
            ImGui::ColorEdit3("Point Light Specular", glm::value_ptr(uiState.lightSettings.pointSpecular));
            ImGui::DragFloat("Light Brightness", &uiState.lightSettings.pointIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::DragFloat("Ambient Brightness", &uiState.lightSettings.pointAmbientIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::SliderFloat("Point Shadow Strength", &uiState.lightSettings.pointShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Point Light Constant", &uiState.lightSettings.pointConstant, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Point Light Linear", &uiState.lightSettings.pointLinear, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Point Light Quadratic", &uiState.lightSettings.pointQuadratic, 0.001f, 0.0f, 1.0f);
        }

        if (uiState.sceneConfig.enableDirectionalLight)
        {
            ImGui::SeparatorText("Directional Light Settings");

            constexpr ImGuiColorEditFlags lightColorFlags =
                ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
            if (ImGui::ColorEdit3(
                    "Directional Light Color",
                    glm::value_ptr(uiState.lightSettings.sunDiffuse),
                    lightColorFlags))
            {
                uiState.lightSettings.sunSpecular = uiState.lightSettings.sunDiffuse;
            }
            ImGui::ColorEdit3(
                "Directional Light Ambient",
                glm::value_ptr(uiState.lightSettings.sunAmbient),
                lightColorFlags);
            ImGui::DragFloat("Directional Light Intensity", &uiState.lightSettings.sunIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::SliderFloat("Sun Intensity", &uiState.lightSettings.sunIntensityScale, 0.0f, 5.0f, "%.2f");
            ImGui::SliderFloat("Directional Shadow Strength", &uiState.lightSettings.sunShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("PCSS Light Size", &uiState.sceneConfig.directionalShadowLightSize, 0.0f, 0.03f, "%.4f");
            ImGui::SliderFloat("PCSS Blocker Search", &uiState.sceneConfig.directionalShadowBlockerSearchRadius, 0.0f, 0.03f, "%.4f");
            ImGui::SliderFloat("PCSS Min Filter", &uiState.sceneConfig.directionalShadowMinFilterRadius, 0.0f, 0.01f, "%.4f");
            ImGui::SliderFloat("PCSS Max Filter", &uiState.sceneConfig.directionalShadowMaxFilterRadius, 0.0f, 0.04f, "%.4f");
            ImGui::SliderFloat("Directional Bias Slope", &uiState.sceneConfig.directionalShadowBiasSlope, 0.0f, 0.05f, "%.5f");
            ImGui::SliderFloat("Directional Bias Min", &uiState.sceneConfig.directionalShadowBiasMin, 0.0f, 0.005f, "%.5f");
            ImGui::Text("Sun from HDR: %s", uiState.lightSettings.sunExtractedFromEnvironment ? "yes" : "no");
        }

        if (uiState.sceneConfig.enableFlashlight)
        {
            ImGui::SeparatorText("Flashlight Settings");

            ImGui::ColorEdit3("Flashlight Ambient", glm::value_ptr(uiState.lightSettings.flashAmbient));
            ImGui::ColorEdit3("Flashlight Diffuse", glm::value_ptr(uiState.lightSettings.flashDiffuse));
            ImGui::ColorEdit3("Flashlight Specular", glm::value_ptr(uiState.lightSettings.flashSpecular));
            ImGui::DragFloat("Flashlight Intensity", &uiState.lightSettings.flashIntensity, 0.05f, 0.0f, 20.0f);
            ImGui::SliderFloat("Flashlight Shadow Strength", &uiState.lightSettings.flashShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Flashlight Constant", &uiState.lightSettings.flashConstant, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight Linear", &uiState.lightSettings.flashLinear, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight Quadratic", &uiState.lightSettings.flashQuadratic, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight CutOff", &uiState.lightSettings.flashCutOff, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("Flashlight OuterCutOff", &uiState.lightSettings.flashOuterCutOff, 0.001f, 0.0f, 1.0f);
        }

        ImGui::End();
    }
