#include "ui_import.h"
#include <algorithm>
#include <cctype>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <string>
#include "rendering/resources/environment/EnvironmentOption.h"
#include "rendering/uniforms/VegetationExposure.h"

void SceneRenderUI::renderUI(
    SceneRenderUIState& uiState,
    float FPS,
    float swapWaitMs)
{
     ImGui::Begin("Deferred PBR Renderer");
        ImGui::Text("FPS: %.2f", FPS);
        ImGui::Text("Swap wait ms: %.3f", swapWaitMs);
        if (uiState.gpuProfiler &&
            ImGui::CollapsingHeader("GPU Pass Timings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const std::vector<GpuPassTiming> timings =
                uiState.gpuProfiler->timings();
            double totalMilliseconds = 0.0;
            for (const GpuPassTiming& timing : timings)
            {
                if (!timing.valid)
                    continue;
                totalMilliseconds += timing.milliseconds;
                ImGui::Text("%-22s %7.3f ms",
                            timing.name.c_str(), timing.milliseconds);
            }
            ImGui::Separator();
            ImGui::Text("Measured GPU total      %7.3f ms", totalMilliseconds);
            ImGui::TextDisabled("Smoothed async GL_TIME_ELAPSED results");
        }
        if (uiState.environmentLoadFailed)
        {
            ImGui::TextColored(
                ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                "Environment load failed! Check console.");
        }

        ImGui::SetNextItemWidth(-72.0f);
        ImGui::InputTextWithHint(
            "##SettingsSearch",
            "Search settings... (e.g. wind, vegetation, TAA)",
            settingsSearch_.data(),
            settingsSearch_.size());
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            settingsSearch_.fill('\0');

        if (settingsSearch_[0] != '\0')
        {
            const auto foldAscii = [](std::string value)
            {
                std::transform(
                    value.begin(), value.end(), value.begin(),
                    [](unsigned char c)
                    {
                        return c < 128
                            ? static_cast<char>(std::tolower(c))
                            : static_cast<char>(c);
                    });
                return value;
            };
            const std::string foldedQuery =
                foldAscii(std::string(settingsSearch_.data()));
            const auto matches = [&](const char* searchableText)
            {
                const std::string haystack =
                    foldAscii(std::string(searchableText));
                std::istringstream tokens(foldedQuery);
                std::string token;
                bool hadToken = false;
                while (tokens >> token)
                {
                    hadToken = true;
                    if (haystack.find(token) == std::string::npos)
                        return false;
                }
                return hadToken;
            };

            ImGui::SeparatorText("Search Results");
            int resultCount = 0;
            if (matches(
                    "Foliage Mip Bias vegetation alpha texture mip shimmer "
                    "植被 纹理 透明度 闪烁"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Foliage Mip Bias##Search",
                    &uiState.sceneConfig.vegetationMaterialMipBias,
                    -0.5f, 1.5f, "%.2f");
            }
            if (matches(
                    "Foliage Transmission vegetation lighting leaf light "
                    "植被 透射 光照 叶片"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Foliage Transmission##Search",
                    &uiState.sceneConfig.vegetationTransmissionStrength,
                    0.0f, 1.5f, "%.2f");
            }
            if (matches(
                    "Vegetation Exposure Coefficient foliage lighting "
                    "植被 曝光 光照"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Vegetation Exposure Coefficient##Search",
                    &uiState.sceneConfig.vegetationExposureCoefficient,
                    0.25f, 2.0f, "%.2f");
            }
            if (matches(
                    "Vegetation Wind Direction wind foliage grass "
                    "植被 风向 风 草"))
            {
                ++resultCount;
                ImGui::SliderFloat2(
                    "Vegetation Wind Direction##Search",
                    glm::value_ptr(
                        uiState.sceneConfig.vegetationWindDirection),
                    -1.0f, 1.0f, "%.2f");
            }
            if (matches(
                    "Vegetation Wind Speed wind animation foliage "
                    "植被 风速 风 动画"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Vegetation Wind Speed##Search",
                    &uiState.sceneConfig.vegetationWindSpeed,
                    0.0f, 4.0f, "%.2f");
            }
            if (matches(
                    "Vegetation Wind Strength wind animation foliage "
                    "植被 风力 强度 风 动画"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Vegetation Wind Strength##Search",
                    &uiState.sceneConfig.vegetationWindStrength,
                    0.0f, 1.2f, "%.2f m");
            }
            if (matches(
                    "Grass Draw Distance vegetation range LOD grass "
                    "植被 草 距离 范围"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Grass Draw Distance##Search",
                    &uiState.sceneConfig.vegetationGrassDistance,
                    40.0f, 220.0f, "%.0f m");
            }
            if (matches(
                    "Flower Draw Distance vegetation range LOD flower "
                    "植被 花 距离 范围"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Flower Draw Distance##Search",
                    &uiState.sceneConfig.vegetationFlowerDistance,
                    30.0f, 240.0f, "%.0f m");
            }
            if (matches(
                    "Tree Draw Distance vegetation range LOD tree "
                    "植被 树 距离 范围"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "Tree Draw Distance##Search",
                    &uiState.sceneConfig.vegetationTreeDistance,
                    300.0f, 2400.0f, "%.0f m");
            }
            if (matches(
                    "Temporal AA TAA antialiasing post process "
                    "时域抗锯齿 抗锯齿"))
            {
                ++resultCount;
                ImGui::Checkbox(
                    "Temporal AA##Search",
                    &uiState.sceneConfig.enableTAA);
            }
            if (matches(
                    "TAA History Weight temporal history accumulation "
                    "时域 历史 权重"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "TAA History Weight##Search",
                    &uiState.sceneConfig.taaHistoryWeight,
                    0.0f, 0.96f, "%.2f");
            }
            if (matches(
                    "TAA Sharpness temporal sharpen post process "
                    "时域 锐化 清晰度"))
            {
                ++resultCount;
                ImGui::SliderFloat(
                    "TAA Sharpness##Search",
                    &uiState.sceneConfig.taaSharpness,
                    0.0f, 0.75f, "%.2f");
            }
            if (matches(
                    "Directional Light sunlight sun lighting shadow "
                    "方向光 太阳 光照 阴影"))
            {
                ++resultCount;
                ImGui::Checkbox(
                    "Directional Light##Search",
                    &uiState.sceneConfig.enableDirectionalLight);
            }
            if (matches(
                    "PBR physically based rendering material lighting "
                    "物理渲染 材质 光照"))
            {
                ++resultCount;
                ImGui::Checkbox("PBR##Search", &uiState.sceneConfig.enablePBR);
            }
            if (matches(
                    "IBL image based lighting environment reflection "
                    "环境光 图像照明 反射"))
            {
                ++resultCount;
                ImGui::Checkbox("IBL##Search", &uiState.sceneConfig.enableIBL);
            }
            if (resultCount == 0)
                ImGui::TextDisabled("No matching setting.");
            else
                ImGui::TextDisabled("%d matching setting(s)", resultCount);

            ImGui::End();
            return;
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
        if (ImGui::TreeNodeEx("Sunny Island Water", ImGuiTreeNodeFlags_DefaultOpen))
        {
            WaterRenderSettings& water = uiState.sceneConfig.water;
            ImGui::DragFloat2("Water Wind XZ", glm::value_ptr(water.windDirection),
                              0.01f, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Wave Amplitude", &water.waveAmplitude,
                               0.0f, 0.24f, "%.3f m");
            ImGui::SliderFloat("Wavelength Scale", &water.wavelengthScale,
                               0.5f, 2.0f, "%.2f");
            ImGui::SliderFloat("Detail Normal", &water.detailNormalStrength,
                               0.0f, 0.45f, "%.2f");
            ImGui::SliderFloat("Refraction Strength", &water.refractionStrength,
                               0.0f, 0.04f, "%.3f");
            ImGui::ColorEdit3("Scattering Color", glm::value_ptr(water.scatteringColor));
            ImGui::DragFloat3("Absorption RGB", glm::value_ptr(water.absorptionCoefficient),
                              0.002f, 0.0f, 1.0f, "%.3f");
            ImGui::SliderFloat("Max Absorption Distance", &water.maxAbsorptionDistance,
                               1.0f, 60.0f, "%.1f m");
            ImGui::SliderFloat("Water Roughness", &water.roughness,
                               0.04f, 0.18f, "%.3f");
            ImGui::SliderFloat("Foam Shore Width", &water.foamShoreWidth,
                               0.15f, 1.2f, "%.2f m");

            if (ImGui::TreeNodeEx("Caustics", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable Caustics", &water.enableCaustics);
                ImGui::SliderFloat("Caustic Strength", &water.causticStrength,
                                   0.0f, 4.0f, "%.2f");
                ImGui::SliderFloat("Photon Focus Sharpness", &water.causticSharpness,
                                   1.0f, 5.0f, "%.2f");
                ImGui::SliderFloat("Photon Footprint", &water.causticScale,
                                   0.04f, 0.24f, "%.3f");
                ImGui::SliderFloat("Optical Refraction Scale", &water.causticCurvatureScale,
                                   0.05f, 0.8f, "%.2f");
                ImGui::DragFloat("Caustic Start Depth", &water.causticDepthStart,
                                 0.02f, 0.0f, 2.0f, "%.2f m");
                ImGui::DragFloat("Caustic Peak Depth", &water.causticDepthPeak,
                                 0.05f, 0.25f, 8.0f, "%.2f m");
                ImGui::DragFloat("Caustic End Depth", &water.causticDepthEnd,
                                  0.25f, 5.0f, 120.0f, "%.1f m");
                ImGui::SliderFloat("Caustic Absorption", &water.causticAbsorptionScale,
                                   0.0f, 2.0f, "%.2f");
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Dispersion", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable Dispersion", &water.enableDispersion);
                ImGui::SliderFloat("Dispersion Strength", &water.dispersionStrength,
                                   0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Dispersion Blend", &water.dispersionBlend,
                                   0.0f, 1.0f, "%.2f");
                ImGui::SliderFloat("Dispersion Depth Falloff", &water.dispersionDepthFalloff,
                                   0.0f, 0.35f, "%.3f");
                ImGui::SliderFloat("Dispersion Max Pixels", &water.dispersionMaxPixels,
                                   0.0f, 12.0f, "%.2f px");
                ImGui::DragFloat3("Water IOR RGB", glm::value_ptr(water.iorRGB),
                                  0.0001f, 1.30f, 1.38f, "%.4f");
                ImGui::SliderFloat("Spectral Glint", &water.spectralGlintStrength,
                                   0.0f, 0.08f, "%.3f");
                ImGui::TreePop();
            }
            if (ImGui::Button("Reset Island Water Defaults"))
                water = WaterRenderSettings{};
            ImGui::TreePop();
        }

        ImGui::SeparatorText("Sky & Volumetrics");
        if (uiState.sceneConfig.renderMode != RenderMode::Lighting)
            ImGui::TextDisabled(
                "Volumetric clouds and god rays are disabled outside Lighting mode.");
        ImGui::Checkbox("Time of Day", &uiState.sceneConfig.enableTimeOfDay);
        ImGui::SliderFloat("Hour", &uiState.sceneConfig.timeOfDayHours,
                           0.0f, 24.0f, "%.2f h");
        ImGui::SliderFloat("Day Length", &uiState.sceneConfig.dayLengthSeconds,
                           30.0f, 1800.0f, "%.0f s");
        ImGui::Checkbox("Automatic Weather",
                        &uiState.sceneConfig.enableAutomaticWeather);
        ImGui::SliderFloat("Weather Hold Time",
                           &uiState.sceneConfig.automaticWeatherIntervalSeconds,
                           20.0f, 600.0f, "%.0f s");
        ImGui::Checkbox("Pure Color Sky", &uiState.sceneConfig.enableProceduralSky);
        ImGui::ColorEdit3("Sky Background", glm::value_ptr(uiState.sceneConfig.skyTopColor));
        ImGui::Checkbox("Volumetric Clouds", &uiState.sceneConfig.enableVolumetricClouds);
        const char* cloudWeatherNames[] = {"Storm", "Sunny", "Overcast"};
        int cloudWeatherIndex = static_cast<int>(uiState.sceneConfig.cloudWeatherPreset);
        if (ImGui::Combo("Weather Target", &cloudWeatherIndex, cloudWeatherNames, 3))
        {
            uiState.sceneConfig.cloudWeatherPreset =
                static_cast<CloudWeatherPreset>(cloudWeatherIndex);
            ++uiState.sceneConfig.cloudWeatherTransitionRequest;
        }
        ImGui::SameLine();
        if (ImGui::Button("Evolve Again"))
            ++uiState.sceneConfig.cloudWeatherTransitionRequest;
        ImGui::SliderFloat(
            "Weather Transition",
            &uiState.sceneConfig.cloudWeatherTransitionDuration,
            2.0f,
            120.0f,
            "%.1f s");
        ImGui::ProgressBar(
            uiState.sceneConfig.cloudWeatherTransitionProgress,
            ImVec2(-1.0f, 0.0f),
            uiState.sceneConfig.cloudWeatherTransitionProgress < 1.0f
                ? "Weather evolving"
                : "Weather stable");
        if (ImGui::TreeNodeEx("Cloud Shape", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Coverage", &uiState.sceneConfig.cloudCoverage, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Density", &uiState.sceneConfig.cloudDensity, 0.05f, 3.0f, "%.2f");
            ImGui::DragFloat("Base Height", &uiState.sceneConfig.cloudBaseHeight, 10.0f, 100.0f, 8000.0f, "%.0f m");
            ImGui::DragFloat("Layer Thickness", &uiState.sceneConfig.cloudThickness, 10.0f, 100.0f, 4000.0f, "%.0f m");
            ImGui::SliderFloat("Macro Scale", &uiState.sceneConfig.cloudScale, 0.25f, 3.0f, "%.2f");
            ImGui::SliderFloat("Detail Scale", &uiState.sceneConfig.cloudDetailScale, 1.5f, 8.0f, "%.2f");
            ImGui::SliderFloat("Cloud Type", &uiState.sceneConfig.cloudType, 0.0f, 1.0f, "%.2f");
            ImGui::SetItemTooltip("0 = flat stratus, 1 = tall cumulus");
            ImGui::SliderFloat("Anvil Amount", &uiState.sceneConfig.cloudAnvilAmount, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Erosion", &uiState.sceneConfig.cloudErosionStrength, 0.0f, 0.6f, "%.2f");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Cloud Wind", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Speed", &uiState.sceneConfig.cloudSpeed, 0.0f, 40.0f, "%.1f m/s");
            ImGui::SliderFloat("Evolution Speed", &uiState.sceneConfig.cloudEvolutionSpeed, 0.0f, 0.30f, "%.3f");
            ImGui::SetItemTooltip("Continuous cloud-shape morphing; independent of wind translation");
            ImGui::DragFloat2("Direction XZ", glm::value_ptr(uiState.sceneConfig.cloudWindDirection), 0.01f, -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Height Shear", &uiState.sceneConfig.cloudWindShear, -1.0f, 1.0f, "%.2f");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Storm Light Holes", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Hole Strength", &uiState.sceneConfig.stormHoleStrength, 0.0f, 1.0f, "%.2f");
            ImGui::Text("Pattern %u: %d generated holes",
                        uiState.sceneConfig.stormHoleSeed,
                        uiState.sceneConfig.stormHoleCount);
            ImGui::TextWrapped("All generated holes use fixed world-space anchors; the camera cannot drag their light footprints.");
            if (ImGui::Button("Regenerate Hole Pattern"))
                ++uiState.sceneConfig.cloudWeatherTransitionRequest;
            ImGui::DragFloat("Minimum Radius", &uiState.sceneConfig.stormHoleMinRadius, 5.0f, 40.0f, 900.0f, "%.0f m");
            ImGui::DragFloat("Maximum Radius", &uiState.sceneConfig.stormHoleMaxRadius, 10.0f, 300.0f, 2600.0f, "%.0f m");
            ImGui::SliderFloat("Hole Softness", &uiState.sceneConfig.stormHoleSoftness, 0.05f, 0.80f, "%.2f");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Cloud Lighting", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat("Extinction", &uiState.sceneConfig.cloudExtinction, 0.1f, 4.0f, "%.2f");
            ImGui::SliderFloat("Sun Absorption", &uiState.sceneConfig.cloudLightAbsorption, 0.0f, 8.0f, "%.2f");
            ImGui::SliderFloat("Cloud Shadow Opacity", &uiState.sceneConfig.cloudShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::DragFloat("Cloud Shadow Coverage", &uiState.sceneConfig.cloudShadowCoverage, 50.0f, 4000.0f, 12000.0f, "%.0f m");
            ImGui::SetItemTooltip("Full light-space width of the 512^2 R16F optical-depth map.");
            ImGui::SliderInt("Cloud Shadow March Steps", &uiState.sceneConfig.cloudShadowMarchSteps, 4, 8);
            ImGui::SliderInt("Cloud Shadow Scan Slices", &uiState.sceneConfig.cloudShadowScanSlices, 2, 8);
            ImGui::SliderFloat("Ambient Strength", &uiState.sceneConfig.cloudAmbientStrength, 0.0f, 2.0f, "%.2f");
            ImGui::SliderFloat("Powder Effect", &uiState.sceneConfig.cloudPowderStrength, 0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Multiple Scattering", &uiState.sceneConfig.cloudMultiScattering, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Silver Lining", &uiState.sceneConfig.cloudSilverLining, 0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Forward Phase", &uiState.sceneConfig.cloudForwardScattering, 0.0f, 0.9f, "%.2f");
            ImGui::SliderFloat("Backward Phase", &uiState.sceneConfig.cloudBackwardScattering, -0.8f, 0.0f, "%.2f");
            ImGui::ColorEdit3("Cloud Sun Color", glm::value_ptr(uiState.sceneConfig.cloudSunColor));
            ImGui::ColorEdit3("Cloud Bottom Color", glm::value_ptr(uiState.sceneConfig.cloudBottomColor));
            ImGui::ColorEdit3("Cloud Top Color", glm::value_ptr(uiState.sceneConfig.cloudTopColor));
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Cloud Quality"))
        {
            ImGui::SliderFloat("Render Scale", &uiState.sceneConfig.cloudRenderScale,
                               0.25f, 1.0f, "%.2fx");
            ImGui::SetItemTooltip("0.50x renders one quarter as many cloud pixels; full-resolution TAA performs temporal reconstruction");
            ImGui::SliderInt("View Steps", &uiState.sceneConfig.cloudViewSteps, 16, 96);
            ImGui::SliderInt("Light Steps", &uiState.sceneConfig.cloudLightSteps, 2, 8);
            ImGui::DragFloat("Max Distance", &uiState.sceneConfig.cloudMaxDistance, 500.0f, 5000.0f, 150000.0f, "%.0f m");
            ImGui::SeparatorText("World-space Cache");
            ImGui::Checkbox("Acceleration Cache",
                            &uiState.sceneConfig.enableCloudAcceleration);
            ImGui::Checkbox("Occupancy Skipping",
                            &uiState.sceneConfig.enableCloudOccupancySkipping);
            ImGui::Checkbox("Cached Sun Lighting",
                            &uiState.sceneConfig.enableCloudLightCache);
            ImGui::SliderInt("Cache Resolution",
                             &uiState.sceneConfig.cloudCacheResolution,
                             64, 512);
            ImGui::SliderInt("Cache Tile Interval",
                             &uiState.sceneConfig.cloudCacheUpdateInterval,
                             1, 8);
            ImGui::SetItemTooltip("Frames between quadrant updates; a complete refresh takes four updates");
            ImGui::SliderInt("Cached Light Steps",
                             &uiState.sceneConfig.cloudCacheLightSteps,
                             2, 6);
            ImGui::DragFloat("Cache World Size",
                             &uiState.sceneConfig.cloudCacheWorldSize,
                             1000.0f, 40000.0f, 300000.0f, "%.0f m");
            ImGui::Checkbox("Sun-local High Resolution",
                            &uiState.sceneConfig.enableSunLocalCloudCache);
            ImGui::SliderInt("Sun-local Resolution",
                             &uiState.sceneConfig.sunLocalCloudCacheResolution,
                             256, 1024);
            ImGui::DragFloat("Sun-local World Size",
                             &uiState.sceneConfig.sunLocalCloudCacheWorldSize,
                             250.0f, 20000.0f, 32000.0f, "%.0f m");
            ImGui::SliderInt("Sun-local Tiles Per Axis",
                             &uiState.sceneConfig.sunLocalCloudCacheTilesPerAxis,
                             2, 8);
            ImGui::SetItemTooltip("Amortizes animated 512x512 cache updates across frames");
            ImGui::SliderFloat("Occupancy Threshold",
                               &uiState.sceneConfig.cloudOccupancyThreshold,
                               0.001f, 0.08f, "%.3f");
            ImGui::SliderFloat("Empty-space Skip",
                               &uiState.sceneConfig.cloudEmptySkipMultiplier,
                               1.0f, 12.0f, "%.1fx");
            ImGui::TreePop();
        }

        if (uiState.sceneConfig.sceneSelection == SceneSelection::FujiTerrain &&
            ImGui::TreeNodeEx("Alpine Terrain", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const char* debugModes[] = {"Material", "Height", "Slope", "Aspect",
                                        "Curvature", "Moisture", "Snow Retention"};
            ImGui::Combo("TDM Debug", &uiState.sceneConfig.terrainDebugMode,
                         debugModes, 7);
            ImGui::SliderFloat("Grass/Rock Blend Start", &uiState.sceneConfig.terrainGrassEnd,
                               0.0f, 0.8f, "%.3f");
            ImGui::SliderFloat("Grass/Rock Blend End", &uiState.sceneConfig.terrainRockStart,
                               0.0f, 0.9f, "%.3f");
            ImGui::SliderFloat("Rock/Snow Blend Start", &uiState.sceneConfig.terrainSnowStart,
                               0.3f, 1.0f, "%.3f");
            ImGui::SliderFloat("Rock/Snow Blend End", &uiState.sceneConfig.terrainSnowEnd,
                               0.3f, 1.0f, "%.3f");
            ImGui::SliderFloat("Sun-facing Shift", &uiState.sceneConfig.terrainSunHeightShift,
                               0.0f, 0.15f, "%.3f");
            ImGui::SliderFloat("Boundary Noise", &uiState.sceneConfig.terrainNoiseHeightShift,
                               0.0f, 0.18f, "%.3f");
            ImGui::SliderFloat("Height Blend", &uiState.sceneConfig.terrainBlendSharpness,
                               0.0f, 0.6f, "%.3f");
            ImGui::SeparatorText("Vegetation");
            ImGui::Checkbox("Enable Alpine Vegetation",
                            &uiState.sceneConfig.enableVegetation);
            ImGui::SliderFloat2("Vegetation Wind Direction",
                                glm::value_ptr(uiState.sceneConfig.vegetationWindDirection),
                                -1.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Vegetation Wind Speed",
                               &uiState.sceneConfig.vegetationWindSpeed,
                               0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Vegetation Wind Strength",
                               &uiState.sceneConfig.vegetationWindStrength,
                               0.0f, 1.2f, "%.2f m");
            ImGui::SliderFloat("Foliage Mip Bias",
                               &uiState.sceneConfig.vegetationMaterialMipBias,
                               -0.5f, 1.5f, "%.2f");
            ImGui::SetItemTooltip(
                "Positive values select smoother alpha/normal mips and reduce "
                "sub-pixel foliage shimmer");
            ImGui::SliderFloat(
                "Foliage Transmission",
                &uiState.sceneConfig.vegetationTransmissionStrength,
                0.0f, 1.5f, "%.2f");
            ImGui::SliderFloat(
                "Vegetation Exposure Coefficient",
                &uiState.sceneConfig.vegetationExposureCoefficient,
                0.25f, 2.0f, "%.2f");
            ImGui::SetItemTooltip(
                "Scales the smoothed exposure driven by the original light "
                "energy; 1.00 is neutral for all vegetation");
            ImGui::Text("Effective coefficient: %.2f",
                        calculateVegetationExposureCoefficient(
                            uiState.sceneConfig));
            ImGui::SliderFloat("Grass Draw Distance",
                               &uiState.sceneConfig.vegetationGrassDistance,
                               40.0f, 220.0f, "%.0f m");
            ImGui::SliderFloat("Flower Draw Distance",
                               &uiState.sceneConfig.vegetationFlowerDistance,
                               30.0f, 240.0f, "%.0f m");
            ImGui::SliderFloat("Tree Draw Distance",
                               &uiState.sceneConfig.vegetationTreeDistance,
                               300.0f, 2400.0f, "%.0f m");
            ImGui::TreePop();
        }
        if (ImGui::TreeNodeEx("Volumetric Sun Scattering", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Enable Scattering", &uiState.sceneConfig.enableGodRays);
            ImGui::SliderFloat("Render Scale", &uiState.sceneConfig.volumetricLightRenderScale, 0.25f, 1.0f, "%.2fx");
            ImGui::SliderInt("Ray Steps", &uiState.sceneConfig.volumetricLightSteps, 16, 64);
            ImGui::SliderFloat("HG Anisotropy", &uiState.sceneConfig.volumetricLightAnisotropy, 0.50f, 0.76f, "%.2f");
            ImGui::DragFloat("Extinction Sigma T", &uiState.sceneConfig.volumetricLightExtinction, 0.00001f, 0.00005f, 0.02f, "%.6f /m");
            ImGui::SetItemTooltip("Upper bound for extinction; the pass automatically preserves about 30%% view transmittance up to the current cloud base");
            ImGui::DragFloat("Scattering Sigma S", &uiState.sceneConfig.volumetricLightScattering, 0.00001f, 0.0f, 0.02f, "%.6f /m");
            ImGui::DragFloat("Max March Distance", &uiState.sceneConfig.volumetricLightMaxDistance, 50.0f, 100.0f, 50000.0f, "%.0f m");
            ImGui::SliderFloat("Weather-normalized Intensity", &uiState.sceneConfig.volumetricLightIntensity, 0.0f, 8.0f, "%.2f");
            ImGui::SetItemTooltip("A single artistic control shared by Sunny and Storm; weather contrast is normalized automatically");
            ImGui::SliderFloat("Depth Rejection", &uiState.sceneConfig.volumetricLightDepthSigma, 0.001f, 0.05f, "%.3f");
            ImGui::ColorEdit3("Sun Scatter Tint", glm::value_ptr(uiState.sceneConfig.godRayColor));
            ImGui::TreePop();
        }
        ImGui::Checkbox("Sun Texture", &uiState.sceneConfig.enableSunTexture);
        ImGui::SliderFloat("Sun Angular Radius", &uiState.sceneConfig.sunAngularRadius, 0.015f, 0.16f, "%.3f");
        ImGui::SliderFloat("Solar Glare Intensity", &uiState.sceneConfig.stormHoleShaftStrength, 0.0f, 4.0f, "%.2f");
        ImGui::SetItemTooltip("Independent lens-facing glare control; actual glare is multiplied by the mip-averaged visible area of the solar disk");

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
        ImGui::SliderFloat("Parallax Height Scale", &uiState.renderParams.parallaxHeightScale, 0.0f, 0.02f, "%.3f");
        ImGui::SliderFloat("Bump Normal Strength", &uiState.renderParams.bumpNormalStrength, 0.0f, 10.0f, "%.2f");
        ImGui::SliderInt("Parallax Layers", &uiState.renderParams.numLayers, 1, 64);

        ImGui::SeparatorText("Post Process");
        ImGui::Checkbox("Temporal AA", &uiState.sceneConfig.enableTAA);
        ImGui::SliderFloat("TAA History Weight", &uiState.sceneConfig.taaHistoryWeight, 0.0f, 0.96f, "%.2f");
        ImGui::SliderFloat("TAA Sharpness", &uiState.sceneConfig.taaSharpness, 0.0f, 0.75f, "%.2f");
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
