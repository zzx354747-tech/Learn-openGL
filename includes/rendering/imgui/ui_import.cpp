#include "ui_import.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include "rendering/editor/EditorTransform.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include "rendering/resources/environment/EnvironmentOption.h"

namespace
{
const ImVec4 accent(0.30f, 0.78f, 0.73f, 1.0f);
const ImVec4 muted(0.49f, 0.55f, 0.63f, 1.0f);
constexpr ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

void caption(const char* text)
{
    ImGui::TextColored(muted, "%s", text);
}

void panel(const char* name, ImVec2 position, ImVec2 size)
{
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(size);
    ImGui::Begin(name, nullptr, panelFlags);
}

bool section(const char* name, bool open = true)
{
    ImGui::Spacing();
    return ImGui::CollapsingHeader(name, open ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

void splitter(const char* id, ImVec2 position, float height, float& size, float direction)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(1, 1));
    ImGui::SetNextWindowPos(position);
    ImGui::SetNextWindowSize(ImVec2(5, height));
    ImGui::Begin(id, nullptr, panelFlags | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::InvisibleButton("##resize", ImVec2(5, height));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        ImGui::GetWindowDrawList()->AddRectFilled(position,
            ImVec2(position.x + 5, position.y + height), ImGui::ColorConvertFloat4ToU32(accent));
    }
    if (ImGui::IsItemActive() && !ImGui::IsItemActivated())
        size += ImGui::GetIO().MouseDelta.x * direction;
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Drag to resize panel");
    ImGui::End();
    ImGui::PopStyleVar(2);
}

// Each property owns a stable ID and a two-column row, including long labels.
namespace Property
{
void begin(const char* label)
{
    ImGui::PushID(label);
    ImGui::BeginTable("##property", 2, ImGuiTableFlags_SizingStretchProp);
    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, 0.47f);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.53f);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextUnformatted(label);
    ImGui::PopTextWrapPos();
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
}
void end()
{
    ImGui::EndTable();
    ImGui::PopID();
}
bool Checkbox(const char* label, bool* value)
{
    begin(label);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool changed = ImGui::InvisibleButton("##value", ImVec2(36, 27), ImGuiButtonFlags_EnableNav);
    if (changed) *value = !*value;
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec4 track = *value ? ImVec4(0.20f, 0.54f, 0.51f, 1) :
        ImGui::GetStyleColorVec4(ImGui::IsItemHovered() ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    draw->AddRectFilled(ImVec2(pos.x, pos.y + 4), ImVec2(pos.x + 36, pos.y + 23),
        ImGui::ColorConvertFloat4ToU32(track), 10);
    draw->AddCircleFilled(ImVec2(pos.x + (*value ? 26.5f : 9.5f), pos.y + 13.5f),
        6.5f, IM_COL32(220, 232, 235, 255));
    if (ImGui::IsItemFocused())
        draw->AddRect(pos, ImVec2(pos.x + 36, pos.y + 27), ImGui::ColorConvertFloat4ToU32(accent), 5);
    end();
    return changed;
}
bool DragFloat(const char* label, float* value, float speed, float min, float max,
               const char* format = "%.3f", bool* editFinished = nullptr)
{
    begin(label);
    bool changed = ImGui::DragFloat("##value", value, speed, min, max, format);
    if (editFinished) *editFinished = ImGui::IsItemDeactivatedAfterEdit();
    end();
    return changed;
}
bool SliderFloat(const char* label, float* value, float min, float max,
                 const char* format = "%.2f")
{
    begin(label);
    bool changed = ImGui::SliderFloat("##value", value, min, max, format);
    end();
    return changed;
}
bool SliderInt(const char* label, int* value, int min, int max)
{
    begin(label);
    bool changed = ImGui::SliderInt("##value", value, min, max);
    end();
    return changed;
}
bool ColorEdit3(const char* label, float* value, ImGuiColorEditFlags flags = 0)
{
    begin(label);
    bool changed = ImGui::ColorEdit3("##value", value, flags | ImGuiColorEditFlags_NoInputs);
    ImGui::SameLine();
    ImGui::TextDisabled("Color");
    end();
    return changed;
}
}

void transformRow(const char* label, glm::vec3& value, float speed)
{
    ImGui::PushID(label);
    caption(label);
    constexpr ImVec4 colors[] = {
        {0.83f, 0.36f, 0.40f, 1.0f},
        {0.39f, 0.72f, 0.49f, 1.0f},
        {0.37f, 0.57f, 0.89f, 1.0f}
    };
    const char* axes[] = {"X", "Y", "Z"};
    if (ImGui::BeginTable("##axes", 3, ImGuiTableFlags_SizingStretchSame))
    {
        for (int i = 0; i < 3; ++i)
        {
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            ImGui::TextColored(colors[i], "%s", axes[i]);
            ImGui::SameLine(0.0f, 5.0f);
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::DragFloat("##axis", &value[i], speed, 0.0f, 0.0f, "%.2f");
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void renderMode(SceneRenderUIState& state)
{
    const char* names[] = {"Deferred lighting", "Forward basic", "Reflection", "Shadow map"};
    switch (state.sceneConfig.renderMode)
    {
    case RenderMode::Basic: state.renderModeIndex = 1; break;
    case RenderMode::Reflection: state.renderModeIndex = 2; break;
    case RenderMode::ShadowDebug: state.renderModeIndex = 3; break;
    default: state.renderModeIndex = 0; break;
    }
    ImGui::SetNextItemWidth(175.0f);
    if (ImGui::Combo("##renderMode", &state.renderModeIndex, names, 4))
    {
        const RenderMode modes[] = {RenderMode::Lighting, RenderMode::Basic,
                                   RenderMode::Reflection, RenderMode::ShadowDebug};
        state.sceneConfig.renderMode = modes[state.renderModeIndex];
        state.sceneConfig.forwardLightMode = state.renderModeIndex == 1 ? ForwardLightMode::Basic :
            state.renderModeIndex == 2 ? ForwardLightMode::Reflect : ForwardLightMode::Light;
    }
}

void environment(SceneRenderUIState& uiState)
{
    const char* names[] = {"Night", "Sunny", "Night N8 / 3K"};
    uiState.environmentIndex = getEnvironmentIndex(uiState.sceneConfig.environmentSelection);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::Combo("##environment", &uiState.environmentIndex, names, 3))
    {
        uiState.sceneConfig.environmentSelection = kEnvironmentOptions[uiState.environmentIndex].selection;
        uiState.environmentLoadFailed = !uiState.loadEnvironment();
        if (!uiState.environmentLoadFailed)
        {
            uiState.applyEnvironmentPreset();
            uiState.renderModeIndex = 0;
        }
    }
    if (uiState.environmentLoadFailed)
        ImGui::TextWrapped("Environment could not be loaded. Choose another environment to retry.");
}

void materialMapping(SceneRenderUIState& uiState)
{
    Property::Checkbox("Normal Mapping", &uiState.renderParams.enableNormalMapping);
    Property::Checkbox("Parallax Mapping", &uiState.renderParams.enableParallaxMapping);
    Property::SliderFloat("Height scale", &uiState.renderParams.parallaxHeightScale, 0.0f, 0.1f, "%.3f");
    Property::SliderFloat("Normal strength", &uiState.renderParams.bumpNormalStrength, 0.0f, 10.0f, "%.2f");
    Property::SliderInt("Parallax Layers", &uiState.renderParams.numLayers, 1, 64);


}
void postProcessing(SceneRenderUIState& uiState)
{
    Property::Checkbox("SSAO", &uiState.sceneConfig.enableSSAO);
    Property::SliderFloat("SSAO Strength", &uiState.sceneConfig.ssaoStrength, 0.0f, 4.0f, "%.2f");
    Property::Checkbox("Bloom", &uiState.sceneConfig.enableBloom);
    Property::SliderFloat("Exposure", &uiState.sceneConfig.exposure, 0.1f, 5.0f, "%.1f");
    Property::SliderFloat("Bloom Strength", &uiState.sceneConfig.bloomStrength, 0.0f, 3.0f, "%.2f");
    Property::DragFloat("Bloom Max Brightness", &uiState.sceneConfig.bloomMaxBrightness, 0.05f, 0.0f, 1000.0f, "%.2f");
    ImGui::TextWrapped("Caps the Bloom source only (0 = unlimited). Level thresholds operate after this cap.");
    Property::SliderInt("Bloom Levels", &uiState.sceneConfig.bloomLevels, 1, 5);
    Property::SliderFloat("Bloom Filter Radius", &uiState.sceneConfig.bloomFilterRadius, 0.0f, 2.0f, "%.2f");
    ImGui::TextWrapped("For a tight halo, use 1 level and a small filter radius. Thresholds select brightness; they do not set halo size.");
    Property::DragFloat("Bloom Threshold", &uiState.sceneConfig.bloomThreshold, 0.05f, 0.0f, 1000.0f, "%.2f");
    Property::DragFloat("Bloom Soft Knee", &uiState.sceneConfig.bloomKnee, 0.01f, 0.0f, 1000.0f, "%.2f");
    ImGui::TextWrapped("Level thresholds: 0 = bypass. Higher levels create wider halos. Filtering a level also affects all coarser levels.");
    const char* levelLabels[] = {"Bloom L1 (1/2)", "Bloom L2 (1/4)", "Bloom L3 (1/8)",
                                 "Bloom L4 (1/16)", "Bloom L5 (1/32)"};
    for (int level = 0; level < 5; ++level)
        Property::DragFloat(levelLabels[level], &uiState.sceneConfig.bloomLevelThresholds[level],
                            0.05f, 0.0f, 1000.0f, "%.2f");


}
void ambientSettings(SceneRenderUIState& uiState)
{
    Property::ColorEdit3("Ambient color", glm::value_ptr(uiState.sceneConfig.fixedAmbientColor));
    Property::DragFloat("Ambient strength", &uiState.sceneConfig.fixedAmbientStrength, 0.01f, 0.0f, 4.0f);
    Property::ColorEdit3("IBL tint", glm::value_ptr(uiState.sceneConfig.iblAmbientTint));
    Property::DragFloat("IBL strength", &uiState.sceneConfig.iblAmbientStrength, 0.01f, 0.0f, 4.0f);


}
void phongSettings(SceneRenderUIState& uiState)
{
    Property::DragFloat("Diffuse", &uiState.sceneConfig.phongDiffuseStrength, 0.01f, 0.0f, 2.0f);
    Property::DragFloat("Specular", &uiState.sceneConfig.phongSpecularStrength, 0.01f, 0.0f, 2.0f);
    Property::DragFloat("IBL diffuse", &uiState.sceneConfig.phongIBLDiffuseStrength, 0.01f, 0.0f, 6.0f);
    Property::DragFloat("IBL specular", &uiState.sceneConfig.phongIBLSpecularStrength, 0.01f, 0.0f, 4.0f);


}
void lightSettings(SceneRenderUIState& uiState)
{
    Property::Checkbox("Point Light", &uiState.sceneConfig.enablePointLight);
    Property::Checkbox("Directional Light", &uiState.sceneConfig.enableDirectionalLight);
    Property::Checkbox("Flashlight", &uiState.sceneConfig.enableFlashlight);
    bool thresholdEdited = false;
    Property::DragFloat("Sun Threshold", &uiState.sceneConfig.sunThreshold, 1.0f, 0.0f, 1000.0f, "%.1f", &thresholdEdited);
    if (thresholdEdited)
    {
        // 按照代码顺序进行检测
        uiState.environmentLoadFailed = !uiState.loadEnvironment();
    }

    if (uiState.sceneConfig.enablePointLight)
    {
        ImGui::SeparatorText("Point Light");

        Property::ColorEdit3("Point ambient", glm::value_ptr(uiState.lightSettings.pointAmbient));
        Property::ColorEdit3("Point diffuse", glm::value_ptr(uiState.lightSettings.pointDiffuse));
        Property::ColorEdit3("Point specular", glm::value_ptr(uiState.lightSettings.pointSpecular));
        Property::DragFloat("Light Brightness", &uiState.lightSettings.pointIntensity, 0.05f, 0.0f, 20.0f);
        Property::DragFloat("Ambient Brightness", &uiState.lightSettings.pointAmbientIntensity, 0.05f, 0.0f, 20.0f);
        Property::SliderFloat("Point Shadow Strength", &uiState.lightSettings.pointShadowStrength, 0.0f, 1.0f, "%.2f");
        Property::DragFloat("Constant attenuation", &uiState.lightSettings.pointConstant, 0.01f, 0.0f, 1.0f);
        Property::DragFloat("Linear attenuation", &uiState.lightSettings.pointLinear, 0.001f, 0.0f, 1.0f);
        Property::DragFloat("Quadratic attenuation", &uiState.lightSettings.pointQuadratic, 0.001f, 0.0f, 1.0f);
    }

    if (uiState.sceneConfig.enableDirectionalLight)
    {
        ImGui::SeparatorText("Directional Light");

        constexpr ImGuiColorEditFlags lightColorFlags =
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
        if (Property::ColorEdit3(
                "Sun color",
                glm::value_ptr(uiState.lightSettings.sunDiffuse),
                lightColorFlags))
        {
            uiState.lightSettings.sunSpecular = uiState.lightSettings.sunDiffuse;
        }
        Property::ColorEdit3(
            "Sun ambient",
            glm::value_ptr(uiState.lightSettings.sunAmbient),
            lightColorFlags);
        Property::DragFloat("Light intensity", &uiState.lightSettings.sunIntensity, 0.05f, 0.0f, 20.0f);
        Property::SliderFloat("Sun Intensity", &uiState.lightSettings.sunIntensityScale, 0.0f, 5.0f, "%.2f");
        Property::SliderFloat("Sun shadow strength", &uiState.lightSettings.sunShadowStrength, 0.0f, 1.0f, "%.2f");
        Property::SliderFloat("PCSS Light Size", &uiState.sceneConfig.directionalShadowLightSize, 0.0f, 0.03f, "%.4f");
        Property::SliderFloat("PCSS Blocker Search", &uiState.sceneConfig.directionalShadowBlockerSearchRadius, 0.0f, 0.03f, "%.4f");
        Property::SliderFloat("PCSS Min Filter", &uiState.sceneConfig.directionalShadowMinFilterRadius, 0.0f, 0.01f, "%.4f");
        Property::SliderFloat("PCSS Max Filter", &uiState.sceneConfig.directionalShadowMaxFilterRadius, 0.0f, 0.04f, "%.4f");
        Property::SliderFloat("Bias slope", &uiState.sceneConfig.directionalShadowBiasSlope, 0.0f, 0.05f, "%.5f");
        Property::SliderFloat("Minimum bias", &uiState.sceneConfig.directionalShadowBiasMin, 0.0f, 0.005f, "%.5f");
        ImGui::Text("Sun from HDR: %s", uiState.lightSettings.sunExtractedFromEnvironment ? "yes" : "no");
    }

    if (uiState.sceneConfig.enableFlashlight)
    {
        ImGui::SeparatorText("Flashlight");

        Property::ColorEdit3("Flash ambient", glm::value_ptr(uiState.lightSettings.flashAmbient));
        Property::ColorEdit3("Flash diffuse", glm::value_ptr(uiState.lightSettings.flashDiffuse));
        Property::ColorEdit3("Flash specular", glm::value_ptr(uiState.lightSettings.flashSpecular));
        Property::DragFloat("Flashlight Intensity", &uiState.lightSettings.flashIntensity, 0.05f, 0.0f, 20.0f);
        Property::SliderFloat("Flash shadow strength", &uiState.lightSettings.flashShadowStrength, 0.0f, 1.0f, "%.2f");
        Property::DragFloat("Flash constant", &uiState.lightSettings.flashConstant, 0.01f, 0.0f, 1.0f);
        Property::DragFloat("Flash linear", &uiState.lightSettings.flashLinear, 0.001f, 0.0f, 1.0f);
        Property::DragFloat("Flash quadratic", &uiState.lightSettings.flashQuadratic, 0.001f, 0.0f, 1.0f);
        Property::DragFloat("Inner angle", &uiState.lightSettings.flashCutOff, 0.001f, 0.0f, 1.0f);
        Property::DragFloat("Outer angle", &uiState.lightSettings.flashOuterCutOff, 0.001f, 0.0f, 1.0f);
    }


}
} // namespace

void SceneRenderUI::configureStyle()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.FontSizeBase = 15.0f;
    s.WindowPadding = ImVec2(16, 14);
    s.FramePadding = ImVec2(9, 6);
    s.ItemSpacing = ImVec2(8, 8);
    s.ItemInnerSpacing = ImVec2(6, 5);
    s.CellPadding = ImVec2(0, 4);
    s.WindowRounding = 7;
    s.ChildRounding = 7;
    s.FrameRounding = 5;
    s.PopupRounding = 7;
    s.ScrollbarRounding = 8;
    s.GrabRounding = 4;
    s.TabRounding = 4;
    s.WindowBorderSize = 1;
    s.FrameBorderSize = 0;
    s.ScrollbarSize = 11;
    s.GrabMinSize = 9;
    s.SeparatorTextPadding = ImVec2(0, 8);
    auto* c = s.Colors;
    c[ImGuiCol_Text] = ImVec4(0.86f, 0.89f, 0.93f, 1);
    c[ImGuiCol_TextDisabled] = muted;
    c[ImGuiCol_WindowBg] = ImVec4(0.075f, 0.088f, 0.11f, 1);
    c[ImGuiCol_ChildBg] = ImVec4(0.065f, 0.077f, 0.095f, 1);
    c[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.12f, 0.15f, 1);
    c[ImGuiCol_Border] = ImVec4(0.16f, 0.19f, 0.23f, 1);
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.145f, 0.18f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.22f, 0.27f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.29f, 0.31f, 1);
    c[ImGuiCol_CheckMark] = accent;
    c[ImGuiCol_SliderGrab] = accent;
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.48f, 0.91f, 0.84f, 1);
    c[ImGuiCol_Button] = ImVec4(0.14f, 0.18f, 0.22f, 1);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.30f, 0.33f, 1);
    c[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.39f, 0.39f, 1);
    c[ImGuiCol_Header] = ImVec4(0.13f, 0.17f, 0.21f, 1);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.26f, 0.29f, 1);
    c[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.33f, 0.34f, 1);
    c[ImGuiCol_Tab] = ImVec4(0.09f, 0.11f, 0.14f, 1);
    c[ImGuiCol_TabHovered] = ImVec4(0.18f, 0.28f, 0.30f, 1);
    c[ImGuiCol_TabSelected] = ImVec4(0.15f, 0.22f, 0.25f, 1);
    c[ImGuiCol_TabSelectedOverline] = accent;
    c[ImGuiCol_Separator] = c[ImGuiCol_Border];
    c[ImGuiCol_ResizeGrip] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.23f, 0.28f, 0.33f, 1);
    c[ImGuiCol_TextSelectedBg] = ImVec4(0.24f, 0.62f, 0.59f, 0.35f);
    ImFontConfig font;
    font.SizePixels = 15.0f;
    const char* fonts[] = {
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };
    ImFont* loadedFont = nullptr;
    for (const char* path : fonts)
    {
        if (std::ifstream(path).good())
        {
            loadedFont = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, 15.0f);
            if (loadedFont) break;
        }
    }
    if (!loadedFont) loadedFont = ImGui::GetIO().Fonts->AddFontDefaultVector(&font);
    ImGui::GetIO().FontDefault = loadedFont;
}

void SceneRenderUI::renderUI(
    SceneRenderUIState& uiState,
    SceneObjectManager& manager,
    SceneObjectID& selectedObjectID,
    float FPS,
    float swapWaitMs,
    unsigned int viewportTexture,
    bool cameraCaptured,
    Camera& camera)
{
    const ImGuiViewport* screen = ImGui::GetMainViewport();
    const ImVec2 origin = screen->Pos;
    const float width = screen->Size.x;
    const float height = screen->Size.y;
    const float top = 58.0f;
    const float bottom = 30.0f;
    const float gap = 5.0f;
    const float bodyHeight = std::max(1.0f, height - top - bottom - gap * 2);
    scenePanelWidth = std::clamp(scenePanelWidth, 185.0f, std::max(185.0f, width * 0.25f));
    inspectorWidth = std::clamp(inspectorWidth, 290.0f, std::max(290.0f, width * 0.36f));
    const float left = showScenePanel ? scenePanelWidth : 0.0f;
    const float right = showInspector ? inspectorWidth : 0.0f;
    const float centerX = left > 0 ? left + gap : 0;
    const float centerWidth = std::max(1.0f, width - centerX - (right > 0 ? right + gap : 0));
    auto& config = uiState.sceneConfig;
    viewportClicked = false;
    viewportVisible = false;
    gizmoHovered = false;
    gizmoUsing = false;
    importWidth = importHeight = 0;
    if (!cameraCaptured && !ImGuizmo::IsUsingAny() && !ImGui::GetIO().WantTextInput && !ImGui::IsAnyItemActive())
    {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoOperation = 0;
        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoOperation = 1;
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation = 2;
    }

    // Global command strip: only commands supported by the renderer are exposed.
    panel("##editorToolbar", origin, ImVec2(width, top));
    const ImVec2 mark = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRectFilled(mark, ImVec2(mark.x + 30, mark.y + 30),
        IM_COL32(36, 90, 90, 255), 7);
    ImGui::GetWindowDrawList()->AddText(ImVec2(mark.x + 10, mark.y + 6),
        ImGui::ColorConvertFloat4ToU32(accent), "R");
    ImGui::Dummy(ImVec2(30, 30));
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("RENDER STUDIO");
    ImGui::SameLine(0, 22);
    renderMode(uiState);
    ImGui::SameLine(0, 16);
    ImGui::Checkbox("Edit", &config.enableEdit);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Grid and selection outline  /  shortcut: 8");
    ImGui::SameLine();
    ImGui::Checkbox("Sky", &config.enableSkybox);
    ImGui::SameLine();
    if (ImGui::Button("File")) ImGui::OpenPopup("##sceneFile");
    if (ImGui::IsItemHovered() && !uiState.sceneFileMessage.empty())
        ImGui::SetTooltip("%s", uiState.sceneFileMessage.c_str());
    if (ImGui::BeginPopup("##sceneFile"))
    {
        if (ImGui::MenuItem("Save scene", nullptr, false, static_cast<bool>(uiState.saveSceneToFile)))
        {
            uiState.sceneFileFailed = !uiState.saveSceneToFile();
        }
        if (ImGui::MenuItem("Load scene", nullptr, false, static_cast<bool>(uiState.loadSceneFromFile)))
        {
            uiState.sceneFileFailed = !uiState.loadSceneFromFile();
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(), width - 105.0f));
    if (ImGui::Button("Layout", ImVec2(85, 0))) ImGui::OpenPopup("##layout");
    if (ImGui::BeginPopup("##layout"))
    {
        ImGui::MenuItem("Scene hierarchy", nullptr, &showScenePanel);
        ImGui::MenuItem("Inspector", nullptr, &showInspector);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset workspace"))
        {
            showScenePanel = showInspector = true;
            scenePanelWidth = 230;
            inspectorWidth = 350;
        }
        ImGui::EndPopup();
    }
    ImGui::End();

    unsigned int sceneObjectCount = 0;
    for (const auto& object : manager.getSceneObjects())
        if (object.scene == config.sceneSelection) ++sceneObjectCount;

    if (showScenePanel)
    {
        panel("##scenePanel", ImVec2(origin.x, origin.y + top + gap), ImVec2(left, bodyHeight));
        caption("WORKSPACE");
        ImGui::Spacing();
        ImGui::TextUnformatted("Scene hierarchy");
        ImGui::Separator();
        int sceneIndex = config.sceneSelection == SceneSelection::LivingRoom ? 1 : 0;
        const char* sceneNames[] = {"Material study", "Living room"};
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##scene", &sceneIndex, sceneNames, 2))
        {
            config.sceneSelection = sceneIndex == 1 ? SceneSelection::LivingRoom : SceneSelection::Default;
            selectedObjectID = InvalidSceneObjectID;
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Filter objects...", objectSearch, sizeof(objectSearch));
        ImGui::Spacing();
        ImGui::TextDisabled("OBJECTS  /  %u", sceneObjectCount);
        ImGuiTextFilter filter(objectSearch);
        // Use ImGui's case-insensitive matching without retaining pointers into editable names.
        ImGui::BeginChild("##objects", ImVec2(0, std::max(60.0f, ImGui::GetContentRegionAvail().y - 230)), ImGuiChildFlags_None);
        unsigned int matches = 0;
        for (const auto& object : manager.getSceneObjects())
        {
            if (object.scene != config.sceneSelection) continue;
            if (!filter.PassFilter(object.name.c_str())) continue;
            ++matches;
            ImGui::PushID(static_cast<int>(object.id));
            const bool sphere = std::holds_alternative<sphereRenderRef>(object.ref);
            const ImVec2 pos = ImGui::GetCursorScreenPos();
            const ImU32 icon = ImGui::ColorConvertFloat4ToU32(object.isVisible ? accent : muted);
            const bool selected = object.id == selectedObjectID;
            std::string name = object.name;
            if (name.rfind("Sphere_", 0) == 0) name = name.substr(7);
            if (name == "model_livingroom") name = "Living room interior";
            ImGui::PushStyleColor(ImGuiCol_Text, object.isVisible ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : muted);
            if (ImGui::Selectable(("     " + name + "##object").c_str(), selected, 0, ImVec2(0, 30)))
                selectedObjectID = object.id;
            ImGui::PopStyleColor();
            if (selected && selectedObjectID != inspectedObject)
                ImGui::SetScrollHereY(0.5f);
            if (sphere)
                ImGui::GetWindowDrawList()->AddCircle(ImVec2(pos.x + 10, pos.y + 15), 5, icon, 16, 1.5f);
            else
                ImGui::GetWindowDrawList()->AddRect(ImVec2(pos.x + 5, pos.y + 10), ImVec2(pos.x + 15, pos.y + 20), icon, 2, 0, 1.5f);
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(object.name.c_str());
                ImGui::TextDisabled("ID %u  /  %s", object.id, object.isVisible ? "Visible" : "Hidden");
                ImGui::EndTooltip();
            }
            ImGui::PopID();
        }
        if (!matches) ImGui::TextWrapped("No objects match your filter.");
        ImGui::EndChild();
        ImGui::Separator();
        caption("MODEL IMPORT");
        const ImVec2 dropMin = ImGui::GetCursorScreenPos();
        const float dropWidth = ImGui::GetContentRegionAvail().x;
        ImGui::InvisibleButton("##modelDrop", ImVec2(dropWidth, 62));
        importX = dropMin.x; importY = dropMin.y;
        importWidth = dropWidth; importHeight = 62;
        auto* dropDraw = ImGui::GetWindowDrawList();
        dropDraw->AddRectFilled(dropMin, ImVec2(dropMin.x + dropWidth, dropMin.y + 62), IM_COL32(28, 48, 52, 255), 6);
        dropDraw->AddRect(dropMin, ImVec2(dropMin.x + dropWidth, dropMin.y + 62), ImGui::ColorConvertFloat4ToU32(accent), 6);
        dropDraw->AddText(ImVec2(dropMin.x + 12, dropMin.y + 12), ImGui::ColorConvertFloat4ToU32(accent), "DROP MODEL FILES");
        dropDraw->AddText(ImVec2(dropMin.x + 12, dropMin.y + 35), ImGui::ColorConvertFloat4ToU32(muted), "GLB / glTF / OBJ / FBX");
        ImGui::SetNextItemWidth(std::max(30.0f, ImGui::GetContentRegionAvail().x - 68));
        const bool enterImport = ImGui::InputTextWithHint("##modelPath", "Or paste a file path...", modelPath,
            sizeof(modelPath), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        const bool importClick = ImGui::Button("Import", ImVec2(60, 0));
        if ((enterImport || importClick) && modelPath[0] && uiState.queueModelImport)
        {
            uiState.queueModelImport(modelPath);
            modelPath[0] = 0;
        }
        ImGui::TextColored(uiState.importFailed ? ImVec4(0.96f, 0.48f, 0.40f, 1) : muted,
            "%s", uiState.importFailed ? "Import failed - hover for details" : "Auto-fit and select on import");
        if (ImGui::IsItemHovered() && !uiState.importMessage.empty())
            ImGui::SetTooltip("%s", uiState.importMessage.c_str());
        ImGui::Separator();
        ImGui::TextDisabled("` Camera on/off   |   8 Edit");
        ImGui::TextDisabled("W / E / R  Move / Rotate / Scale");
        ImGui::End();
    }

    // A true rendered viewport, not a floating control panel over the scene.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    panel("##viewportPanel", ImVec2(origin.x + centerX, origin.y + top + gap), ImVec2(centerWidth, bodyHeight));
    ImGui::SetCursorPos(ImVec2(14, 10));
    ImGui::TextUnformatted("Scene view");
    ImGui::SameLine();
    ImGui::TextDisabled(" / %s", config.sceneSelection == SceneSelection::LivingRoom ? "Living room" : "Material study");
    if (centerWidth > 380)
    {
        ImGui::SameLine(centerWidth - 165);
        ImGui::TextColored(cameraCaptured ? accent : muted, "%s", cameraCaptured ? "CAMERA CAPTURED" : "PERSPECTIVE");
    }
    ImGui::SetCursorPos(ImVec2(12, 38));
    ImGui::BeginDisabled(cameraCaptured || !config.enableEdit);
    const char* tools[] = {"Move", "Rotate", "Scale"};
    for (int i = 0; i < 3; ++i)
    {
        if (i) ImGui::SameLine(0, 4);
        const bool active = gizmoOperation == i;
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.36f, 0.36f, 1));
        if (ImGui::Button(tools[i])) gizmoOperation = i;
        if (active) ImGui::PopStyleColor();
    }
    ImGui::SameLine(0, 10);
    ImGui::BeginDisabled(gizmoOperation == 2);
    const char* spaces[] = {"World", "Local"};
    ImGui::SetNextItemWidth(80);
    ImGui::Combo("##gizmoSpace", &gizmoSpace, spaces, 2);
    ImGui::EndDisabled();
    if (centerWidth > 410)
    {
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &gizmoSnap);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Move: 0.25 units  /  Rotate: 15 degrees  /  Scale: 0.1");
    }
    ImGui::EndDisabled();
    ImGui::SetCursorPos(ImVec2(0, 75));
    const ImVec2 imageMin = ImGui::GetCursorScreenPos();
    const ImVec2 imageSize(std::max(1.0f, centerWidth), std::max(1.0f, bodyHeight - 75));
    viewportX = imageMin.x;
    viewportY = imageMin.y;
    viewportWidth = imageSize.x;
    viewportHeight = imageSize.y;
    ImGui::Image(static_cast<ImTextureID>(viewportTexture), imageSize, ImVec2(0, 1), ImVec2(1, 0));
    viewportVisible = ImGui::IsItemVisible();
    viewportClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left) && !cameraCaptured;
    if (SceneObject* selected = manager.getSceneObject(selectedObjectID))
    {
        const glm::vec4 clip = glm::perspective(glm::radians(45.0f), imageSize.x / imageSize.y, 0.1f, 100.0f) *
            camera.GetViewMatrix() * glm::vec4(selected->transform.position, 1.0f);
        const bool validScale = std::abs(selected->transform.scale.x) > 1e-6f &&
            std::abs(selected->transform.scale.y) > 1e-6f && std::abs(selected->transform.scale.z) > 1e-6f;
        if (selected->isVisible && selected->scene == config.sceneSelection && config.enableEdit && !cameraCaptured &&
            ((clip.w > 0 && clip.z >= -clip.w && validScale) || ImGuizmo::IsUsingAny()))
        {
            const glm::mat4 view = camera.GetViewMatrix();
            const glm::mat4 projection = glm::perspective(glm::radians(45.0f), imageSize.x / imageSize.y, 0.1f, 100.0f);
            glm::mat4 matrix = selected->transform.matrix();
            const ImGuizmo::OPERATION operations[] = {ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALEU};
            const auto operation = operations[gizmoOperation];
            const glm::vec3 snap(gizmoOperation == 0 ? 0.25f : gizmoOperation == 1 ? 15.0f : 0.1f);
            ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
            ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::PushID(static_cast<int>(selectedObjectID));
            ImGuizmo::SetGizmoSizeClipSpace(0.13f);
            ImGuizmo::Enable(true);
            const ImGuizmo::MODE mode = gizmoOperation == 2 || gizmoSpace == 1 ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
            if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation, mode,
                glm::value_ptr(matrix), nullptr, gizmoSnap ? glm::value_ptr(snap) : nullptr))
                EditorTransform::applyMatrix(matrix, selected->transform);
            gizmoUsing = ImGuizmo::IsUsing();
            gizmoHovered = ImGuizmo::IsOver(operation);
            ImGuizmo::PopID();
            if (gizmoUsing || gizmoHovered) viewportClicked = false;
        }
    }
    // Subtle corner badge stays outside the picking logic and never captures input.
    const char* hint = cameraCaptured ? "`  Release camera" : "Drop a model here   /   W E R  Transform";
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 badgeMin(imageMin.x + 14, imageMin.y + imageSize.y - 38);
    const ImVec2 hintSize = ImGui::CalcTextSize(hint);
    if (imageSize.x > hintSize.x + 48 && imageSize.y > 80)
    {
        draw->AddRectFilled(badgeMin, ImVec2(badgeMin.x + hintSize.x + 20, badgeMin.y + 25), IM_COL32(17, 22, 28, 215), 5);
        draw->AddText(ImVec2(badgeMin.x + 10, badgeMin.y + 5), IM_COL32(185, 201, 210, 255), hint);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    SceneObject* object = manager.getSceneObject(selectedObjectID);
    if (object && object->scene != config.sceneSelection)
    {
        selectedObjectID = InvalidSceneObjectID;
        object = nullptr;
    }
    const bool focusObject = object && selectedObjectID != inspectedObject;
    inspectedObject = selectedObjectID;
    if (showInspector)
    {
        panel("##inspector", ImVec2(origin.x + width - right, origin.y + top + gap), ImVec2(right, bodyHeight));
        caption("PROPERTIES");
        ImGui::TextUnformatted("Inspector");
        ImGui::Spacing();
        if (ImGui::BeginTabBar("##inspectorTabs"))
        {
            if (ImGui::BeginTabItem("Object", nullptr, focusObject ? ImGuiTabItemFlags_SetSelected : 0))
            {
                ImGui::BeginChild("##objectProperties", ImVec2(0, 0), ImGuiChildFlags_None);
                if (object)
                {
                    ImGui::Spacing();
                    ImGui::TextColored(accent, "%s", std::holds_alternative<sphereRenderRef>(object->ref) ? "SPHERE INSTANCE" : "MODEL INSTANCE");
                    ImGui::PushTextWrapPos();
                    ImGui::TextUnformatted(object->name.c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::TextDisabled("Object ID  %u", object->id);
                    if (section("Transform"))
                    {
                        transformRow("Position", object->transform.position, 0.01f);
                        transformRow("Rotation", object->transform.rotationDegrees, 0.1f);
                        transformRow("Scale", object->transform.scale, 0.01f);
                        if (ImGui::Button("Reset transform", ImVec2(-1, 0)))
                            ImGui::OpenPopup("Reset transform?");
                        if (ImGui::BeginPopup("Reset transform?"))
                        {
                            ImGui::TextUnformatted("Reset position, rotation and scale?");
                            if (ImGui::Button("Reset"))
                            {
                                object->transform = Transform{};
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }
                    }
                    if (section("Display"))
                    {
                        Property::Checkbox("Visible", &object->isVisible);
                        if (auto* sphere = std::get_if<sphereRenderRef>(&object->ref))
                            ImGui::TextDisabled("Material slot  %u", sphere->materialIndex);
                    }
                }
                else
                {
                    ImGui::Dummy(ImVec2(0, 28));
                    ImGui::TextColored(accent, "Nothing selected");
                    ImGui::Spacing();
                    ImGui::TextWrapped("Select an object in the scene hierarchy or click a surface in the viewport to inspect its transform.");
                    ImGui::Spacing();
                    caption("Hidden objects remain selectable\nin the scene hierarchy.");
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Render"))
            {
                ImGui::BeginChild("##renderProperties", ImVec2(0, 0), ImGuiChildFlags_None);
                if (section("Pipeline"))
                {
                    Property::Checkbox("PBR shading", &config.enablePBR);
                    Property::Checkbox("Image-based lighting", &config.enableIBL);
                    Property::Checkbox("HDR tone mapping", &config.enableHDR);
                    Property::Checkbox("Gamma correction", &config.enableGammaCorrection);
                }
                if (section("Post processing")) postProcessing(uiState);
                if (section("Surface mapping", false))
                {
                    ImGui::TextDisabled("Applies to the rendering pipeline.");
                    materialMapping(uiState);
                }
                if (section("Phong response", false)) phongSettings(uiState);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("World"))
            {
                ImGui::BeginChild("##worldProperties", ImVec2(0, 0), ImGuiChildFlags_None);
                if (section("Environment"))
                {
                    environment(uiState);
                    Property::Checkbox("Show skybox", &config.enableSkybox);
                }
                if (section("Ambient lighting", false)) ambientSettings(uiState);
                if (section("Light settings")) lightSettings(uiState);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    if (showScenePanel)
        splitter("##leftSplitter", ImVec2(origin.x + left, origin.y + top + gap), bodyHeight, scenePanelWidth, 1);
    if (showInspector)
        splitter("##rightSplitter", ImVec2(origin.x + width - right - gap, origin.y + top + gap), bodyHeight, inspectorWidth, -1);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 6));
    panel("##statusBar", ImVec2(origin.x, origin.y + height - bottom), ImVec2(width, bottom));
    const bool statusError = uiState.environmentLoadFailed || uiState.importFailed || uiState.sceneFileFailed;
    const char* status = uiState.environmentLoadFailed ? "ENVIRONMENT ERROR" : uiState.importFailed ? "IMPORT ERROR" :
        uiState.sceneFileFailed ? "SCENE FILE ERROR" :
        !uiState.sceneFileMessage.empty() ?
            (uiState.sceneFileMessage.rfind("Saved", 0) == 0 ? "SCENE SAVED" : "FILE READ ONLY") :
        uiState.importMessage.rfind("Queued:", 0) == 0 ? "IMPORT QUEUED" : "OPENGL  /  READY";
    ImGui::TextColored(statusError ? ImVec4(0.96f, 0.48f, 0.40f, 1) : accent, "%s", status);
    if (ImGui::IsItemHovered())
    {
        if (!uiState.sceneFileMessage.empty() && !uiState.environmentLoadFailed && !uiState.importFailed)
            ImGui::SetTooltip("%s", uiState.sceneFileMessage.c_str());
        else if (!uiState.importMessage.empty())
            ImGui::SetTooltip("%s", uiState.importMessage.c_str());
    }
    if (width > 760)
    {
        ImGui::SameLine(220);
        ImGui::TextDisabled("%u objects   |   %s", sceneObjectCount, object ? object->name.c_str() : "No selection");
        ImGui::SameLine(width - 305);
        ImGui::TextDisabled("%.0f FPS   %.1f ms   |   Swap %.1f ms", FPS,
                            FPS > 0 ? 1000.0f / FPS : 0.0f, swapWaitMs);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}


bool SceneRenderUI::acceptsModelDrop(float x, float y) const
{
    const auto inside = [x, y](float left, float top, float w, float h) {
        return w > 0 && h > 0 && x >= left && x < left + w && y >= top && y < top + h;
    };
    return (viewportVisible && inside(viewportX, viewportY, viewportWidth, viewportHeight)) ||
           inside(importX, importY, importWidth, importHeight);
}
