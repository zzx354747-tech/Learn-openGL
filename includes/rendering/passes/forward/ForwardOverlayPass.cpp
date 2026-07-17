#include "rendering/passes/forward/ForwardOverlayPass.h"
#include "rendering/uniforms/CameraUniformSetter.h"

#include <chrono>

void ForwardOverlayPass::render(
    int bfwidth,
    int bfheight,
    Framebuffer& framebuffer
)
{
    framebuffer.bind();

    glViewport(0, 0, bfwidth, bfheight);
    glEnable(GL_DEPTH_TEST);

    if (config.enableWater && config.sceneSelection == SceneSelection::FujiTerrain)
        captureOpaqueScene(bfwidth, bfheight);

    renderWater(bfwidth, bfheight);

    lightVisualPass::renderLightVisualPass(
        camera,
        resources,
        state,
        config,
        bfwidth,
        bfheight
    );

    framebuffer.unbind();
}

ForwardOverlayPass::ForwardOverlayPass(Camera& camera, SceneRenderResources& resources,
    SceneRenderState& state, SceneRenderConfig& config, LightSettings& lightSettings)
    : camera(camera), resources(resources), state(state), config(config), lightSettings(lightSettings)
{
}

ForwardOverlayPass::~ForwardOverlayPass()
{
    if (sceneColorOpaque)
        glDeleteTextures(1, &sceneColorOpaque);
}

void ForwardOverlayPass::captureOpaqueScene(int width, int height)
{
    if (!sceneColorOpaque)
        glGenTextures(1, &sceneColorOpaque);
    glBindTexture(GL_TEXTURE_2D, sceneColorOpaque);
    if (opaqueWidth != width || opaqueHeight != height)
    {
        opaqueWidth = width;
        opaqueHeight = height;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0,
                     GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void ForwardOverlayPass::renderWater(int bfwidth, int bfheight)
{
    if (!config.enableWater || config.sceneSelection != SceneSelection::FujiTerrain ||
        !resources.waterMesh || !resources.shaderLibrary)
        return;

    Shader& shader = resources.shaderLibrary->water;
    shader.use();
    CameraUniformSetter::apply(shader, camera, bfwidth, bfheight);

    static const auto startTime = std::chrono::steady_clock::now();
    const float time = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - startTime).count();
    shader.setFloat("waterTime", time);
    shader.setVec2("viewportSize", glm::vec2(static_cast<float>(bfwidth), static_cast<float>(bfheight)));
    shader.setVec3("sunDirection", lightSettings.sunDirection);
    const glm::vec3 waterSunColor = config.enableDirectionalLight
        ? lightSettings.sunDiffuse * lightSettings.sunIntensity *
          lightSettings.sunIntensityScale * config.daylightFactor
        : glm::vec3(0.0f);
    shader.setVec3("sunColor", waterSunColor);
    const bool anyLightEnabled = config.enableDirectionalLight ||
                                 config.enablePointLight ||
                                 config.enableFlashlight;
    const float ambientLightFactor = anyLightEnabled
        ? (config.enableIBL
            ? glm::clamp(glm::mix(0.08f, 1.0f, config.daylightFactor), 0.0f, 1.0f)
            : glm::clamp(config.fixedAmbientStrength, 0.0f, 1.0f))
        : 0.0f;
    shader.setFloat("ambientLightFactor", ambientLightFactor);
    shader.setFloat("cloudAmbientTransmission",
                    calculateCloudAmbientTransmission(config));
    shader.setMat3("iblSunRotation", calculateIblSunRotation(lightSettings));
    shader.setMat4("lightSpaceMatrix", state.dirLightSpaceMatrix);
    shader.setFloat("terrainSize", resources.waterMesh->getTerrainSize());
    const WaterRenderSettings& water = config.water;
    shader.setVec2("waterWindDirection", water.windDirection);
    shader.setFloat("waterWaveAmplitude", water.waveAmplitude);
    shader.setFloat("waterWavelengthScale", water.wavelengthScale);
    shader.setFloat("detailNormalStrength", water.detailNormalStrength);
    shader.setFloat("refractionStrength", water.refractionStrength);
    shader.setVec3("absorptionCoefficient", water.absorptionCoefficient);
    shader.setVec3("scatteringColor", water.scatteringColor);
    shader.setFloat("maxAbsorptionDistance", water.maxAbsorptionDistance);
    shader.setFloat("waterRoughness", water.roughness);
    shader.setFloat("foamShoreWidth", water.foamShoreWidth);
    shader.setBool("enableDispersion", water.enableDispersion);
    shader.setVec3("waterIOR_RGB", water.iorRGB);
    shader.setFloat("dispersionStrength", water.dispersionStrength);
    shader.setFloat("dispersionBlend", water.dispersionBlend);
    shader.setFloat("dispersionDepthFalloff", water.dispersionDepthFalloff);
    shader.setFloat("dispersionMaxPixels", water.dispersionMaxPixels);
    shader.setFloat("spectralGlintStrength", water.spectralGlintStrength);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP,
        resources.prefilterMap ? resources.prefilterMap->GetID() : 0);
    shader.setInt("prefilterMap", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D,
        resources.registry.resolveTexture(resources.lightingHandles.gPosition));
    shader.setInt("gPosition", 1);
    shader.setMat4("cloudShadowMatrix", state.cloudShadowMatrix);
    shader.setFloat(
        "cloudShadowFallbackTransmission",
        state.cloudShadowGlobalTransmission);
    shader.setBool(
        "hasCloudOpticalDepthMap",
        resources.shaderLibrary &&
        resources.cloudOpticalDepthTexture != 0 &&
        resources.cloudTransmittanceTexture != 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, resources.cloudOpticalDepthTexture);
    shader.setInt("cloudOpticalDepthMap", 2);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, resources.cloudTransmittanceTexture);
    shader.setInt("cloudTransmittanceMap", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, sceneColorOpaque);
    shader.setInt("sceneColorOpaque", 4);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, resources.waterMesh->getLakeDataTexture());
    shader.setInt("lakeDataMap", 5);
    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D,
        resources.registry.resolveTexture(resources.lightingHandles.shadowMap));
    shader.setInt("shadowMap", 8);

    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteWasEnabled);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    resources.waterMesh->draw(shader);
    glDepthMask(depthWriteWasEnabled);
    if (blendWasEnabled)
        glEnable(GL_BLEND);
}
