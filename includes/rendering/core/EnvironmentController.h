#pragma once
#include "rendering/resources/render/SceneRenderTypes.h"
#include "rendering/assets/texture/HDRTexture.h"
#include "rendering/assets/texture/EnvCubemap.h"
#include "rendering/assets/ibl/IrradianceMap.h"
#include "rendering/assets/ibl/PrefilterMap.h"
#include "rendering/uniforms/RenderParams.h"
#include "rendering/assets/light/LightSettings.h"
#include "rendering/resources/render/SceneRenderResources.h"
#include "core/Shader.h"

class EnvironmentController
{
public:
    EnvironmentController(
        SceneRenderConfig& sceneConfig,
        RenderParams& renderParams,
        LightSettings& lightSettings,
        SceneRenderResources& sceneResources,
        Shader& irradianceShader,
        Shader& prefilterShader
    );

    bool load();
    void applyPreset();

private:
    void applyExtractedSun(const ExtractedLight& extractedSun);

    SceneRenderConfig& sceneConfig_;
    RenderParams& renderParams_;
    LightSettings& lightSettings_;
    SceneRenderResources& sceneResources_;
    Shader& irradianceShader_;
    Shader& prefilterShader_;

    std::unique_ptr<HDRTexture> hdrTexture_;
    std::unique_ptr<EnvCubemap> skybox_;
    std::unique_ptr<IrradianceMap> irradianceMap_;
    std::unique_ptr<PrefilterMap> prefilterMap_;
};