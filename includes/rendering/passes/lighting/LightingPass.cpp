#include "LightingPass.h"
#include "rendering/assets/mesh/TerrainMesh.h"
#include "rendering/assets/mesh/WaterMesh.h"

#include <chrono>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

namespace
{
constexpr int CausticAtlasWidth = 1024;
constexpr int CausticAtlasHeight = 512;
constexpr int PhotonGridResolution = 256;

struct CausticPhotonVertex
{
    glm::vec2 worldXZ;
    float lakeIndex;
};
}

void LightingPass::render(Framebuffer& framebuffer,
    Screenquad& screenQuad)
{
    if (!resources.shaderLibrary)
        return;

    static const auto waterAnimationStart = std::chrono::steady_clock::now();
    currentWaterTime = std::chrono::duration<float>(
        std::chrono::steady_clock::now() - waterAnimationStart).count();
    renderCausticMap(screenQuad);

    framebuffer.bind();
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // 禁止写入深度,保护深度信息

    Shader* shader = getLightingShader();
    shader->use();
    bindLightingInputTextures(*shader);

    setupObjectLighting(*shader);
    setupPointShadowUniform(*shader);
    setupSpotShadowUniform(*shader);

    // 绘制屏幕四边形
    screenQuad.draw();

    // 在draw时如果进行深度测试，会覆盖掉之前的深度信息
    // 在draw之后恢复深度写入，确保后续的skybox和light render能够正确使用深度测试
    glDepthMask(GL_TRUE);
    framebuffer.unbind();
}

void LightingPass::setupObjectLighting(Shader& shader)
{
    shader.setVec3("viewPos", camera.Getposition());
    shader.setBool("enablePointLight", config.enablePointLight);
    shader.setBool("enableDirectionalLight", config.enableDirectionalLight);
    shader.setBool("enableFlashlight", config.enableFlashlight);
    shader.setBool("enableSSAO", config.enableSSAO);
    shader.setBool("enablePBR", config.enablePBR);
    shader.setBool("enableIBL", config.enableIBL);
    shader.setBool("enableGI", config.enableGI);
    const WaterRenderSettings& water = config.water;
    shader.setBool("enableWaterCaustics",
                   config.enableWater && water.enableCaustics &&
                   resources.waterMesh != nullptr);
    shader.setFloat("causticStrength", water.causticStrength);
    shader.setFloat("causticSharpness", water.causticSharpness);
    shader.setFloat("causticDepthStart", water.causticDepthStart);
    shader.setFloat("causticDepthPeak", water.causticDepthPeak);
    shader.setFloat("causticDepthEnd", water.causticDepthEnd);
    shader.setFloat("causticAbsorptionScale", water.causticAbsorptionScale);
    shader.setVec3("waterAbsorptionCoefficient", water.absorptionCoefficient);
    shader.setFloat("terrainSize", resources.waterMesh
        ? resources.waterMesh->getTerrainSize() : 1.0f);
    shader.setFloat("ssaoStrength", config.ssaoStrength);
    shader.setFloat("giStrength", config.giStrength);
    shader.setFloat("giRadius", config.giRadius);
    shader.setFloat("giMaxDistance", config.giMaxDistance);
    shader.setInt("giSampleCount", config.giSampleCount);
    shader.setVec3("fixedAmbientColor", config.fixedAmbientColor);
    shader.setFloat("fixedAmbientStrength", config.fixedAmbientStrength);
    shader.setVec3("iblAmbientTint", config.iblAmbientTint);
    shader.setFloat("iblAmbientStrength", config.iblAmbientStrength *
                    glm::mix(0.08f, 1.0f, config.daylightFactor));
    shader.setFloat("cloudAmbientTransmission",
                    calculateCloudAmbientTransmission(config));
    shader.setFloat("phongDiffuseStrength", config.phongDiffuseStrength);
    shader.setFloat("phongSpecularStrength", config.phongSpecularStrength);
    shader.setFloat("phongIBLDiffuseStrength", config.phongIBLDiffuseStrength);
    shader.setFloat("phongIBLSpecularStrength", config.phongIBLSpecularStrength);
    shader.setFloat("foliageTransmissionStrength",
                    config.vegetationTransmissionStrength);
    shader.setFloat("vegetationExposure", state.vegetationExposure);
    shader.setMat3("iblSunRotation", calculateIblSunRotation(lightSettings));
    shader.setFloat("bloomThreshold", config.bloomThreshold);
    shader.setFloat("pointShadowStrength", lightSettings.pointShadowStrength);
    shader.setFloat("sunShadowStrength", lightSettings.sunShadowStrength);
    shader.setFloat("flashShadowStrength", lightSettings.flashShadowStrength);
    shader.setFloat("directionalShadowLightSize", config.directionalShadowLightSize);
    shader.setFloat("directionalShadowBlockerSearchRadius", config.directionalShadowBlockerSearchRadius);
    shader.setFloat("directionalShadowMinFilterRadius", config.directionalShadowMinFilterRadius);
    shader.setFloat("directionalShadowMaxFilterRadius", config.directionalShadowMaxFilterRadius);
    shader.setFloat("directionalShadowBiasSlope", config.directionalShadowBiasSlope);
    shader.setFloat("directionalShadowBiasMin", config.directionalShadowBiasMin);
    shader.setFloat("waterLevel", resources.waterMesh
        ? resources.waterMesh->getWaterLevel() : 0.0f);
    shader.setFloat("waterTime", currentWaterTime);
    shader.setBool("hasWaterCausticMap", causticResourcesReady);
    shader.setVec4("causticBounds0", causticBounds[0]);
    shader.setVec4("causticBounds1", causticBounds[1]);
    shader.setMat4("lightSpaceMatrix", state.dirLightSpaceMatrix);
    shader.setMat4("cloudShadowMatrix", state.cloudShadowMatrix);
    shader.setFloat(
        "cloudShadowFallbackTransmission",
        state.cloudShadowGlobalTransmission);

    LightUniformSetter::apply(shader, lightSettings, config, state, camera);
}

void LightingPass::setupPointShadowUniform(Shader& shader)
{
    shader.setVec3("lightPos", state.lightPositions);

    if (shadowResources.pointShadowMap)
    {
        shader.setFloat("farPlane", shadowResources.pointShadowMap->getFarPlane());
    }
}

void LightingPass::setupSpotShadowUniform(Shader& shader)
{
    shader.setMat4("spotLightSpaceMatrix", state.spotLightSpaceMatrix);
}

Shader* LightingPass::getLightingShader()
{
    return resources.shaderLibrary ? &resources.shaderLibrary->lightingPass : nullptr;
}

void LightingPass::bindLightingInputTextures(Shader& shader)
{
    const auto& handles = resources.lightingHandles;
    auto& registry = resources.registry;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.gPosition));
    shader.setInt("gPosition", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.gNormalRoughness));
    shader.setInt("gNormalRoughness", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.gAlbedoMetallic));
    shader.setInt("gAlbedoMetallic", 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.ao));
    shader.setInt("AO", 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.shadowMap));
    shader.setInt("shadowMap", 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, registry.resolveTexture(handles.depthCubeMap));
    shader.setInt("depthCubeMap", 5);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.spotShadowMap));
    shader.setInt("spotShadowMap", 6);

    // Keep unit 7 empty. Some Apple OpenGL drivers retain a stale cube target on
    // this unit and report it as unloadable when a float sampler is assigned.
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, registry.resolveTexture(handles.brdfLUT));
    shader.setInt("brdfLUT", 10);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_CUBE_MAP, registry.resolveTexture(handles.irradianceMap));
    shader.setInt("irradianceMap", 8);

    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_CUBE_MAP, registry.resolveTexture(handles.prefilterMap));
    shader.setInt("prefilterMap", 9);

    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(
        GL_TEXTURE_2D,
        shadowResources.cloudOpticalDepthTexture);
    shader.setInt("cloudOpticalDepthMap", 13);

    glActiveTexture(GL_TEXTURE14);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(
        GL_TEXTURE_2D,
        shadowResources.cloudTransmittanceTexture);
    shader.setInt("cloudTransmittanceMap", 14);
    shader.setBool(
        "hasCloudOpticalDepthMap",
        shadowResources.cloudOpticalDepthTexture != 0 &&
        shadowResources.cloudTransmittanceTexture != 0);

    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D,
                  causticResourcesReady ? causticTextures[0] : 0);
    shader.setInt("waterCausticMap", 12);

    glActiveTexture(GL_TEXTURE15);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glBindTexture(GL_TEXTURE_2D, resources.waterMesh
        ? resources.waterMesh->getLakeDataTexture() : 0);
    shader.setInt("lakeDataMap", 15);
}

LightingPass::LightingPass( SceneRenderResources& resources, ShadowResources& shadowResources, SceneRenderConfig& config, SceneRenderState& state, LightSettings& lightSettings, Camera& camera ) : resources(resources), shadowResources(shadowResources), config(config), state(state), lightSettings(lightSettings), camera(camera)
{}

LightingPass::~LightingPass()
{
    if (photonVBO)
        glDeleteBuffers(1, &photonVBO);
    if (photonVAO)
        glDeleteVertexArrays(1, &photonVAO);
    glDeleteTextures(static_cast<GLsizei>(causticTextures.size()),
                     causticTextures.data());
    glDeleteFramebuffers(static_cast<GLsizei>(causticFramebuffers.size()),
                         causticFramebuffers.data());
}

LightingPass::CausticMapStats LightingPass::inspectCausticMap() const
{
    CausticMapStats stats;
    if (!causticResourcesReady || causticFramebuffers[0] == 0u)
        return stats;

    GLint previousReadFramebuffer = 0;
    GLint previousReadBuffer = GL_BACK;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_READ_BUFFER, &previousReadBuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, causticFramebuffers[0]);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    std::vector<glm::vec2> density(
        static_cast<std::size_t>(CausticAtlasWidth) * CausticAtlasHeight);
    glReadPixels(0, 0, CausticAtlasWidth, CausticAtlasHeight,
                 GL_RG, GL_FLOAT, density.data());
    for (const glm::vec2& sample : density)
    {
        stats.maximumDynamicDensity = std::max(
            stats.maximumDynamicDensity, sample.x);
        stats.maximumReferenceDensity = std::max(
            stats.maximumReferenceDensity, sample.y);
        if (sample.y > 0.012f)
        {
            stats.maximumFocusedExcess = std::max(
                stats.maximumFocusedExcess,
                std::max(sample.x - sample.y, 0.0f) /
                    std::max(sample.y, 0.012f));
        }
    }
    stats.valid = glGetError() == GL_NO_ERROR;
    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                      static_cast<unsigned int>(previousReadFramebuffer));
    glReadBuffer(static_cast<unsigned int>(previousReadBuffer));
    return stats;
}

void LightingPass::initializeCausticResources()
{
    if (causticResourcesReady || !resources.waterMesh)
        return;
    const auto& lakes = resources.waterMesh->getLakeRegions();
    if (lakes.size() < 2u)
        return;

    std::vector<CausticPhotonVertex> photons;
    photons.reserve(2u * PhotonGridResolution * PhotonGridResolution);
    for (std::size_t lakeIndex = 0; lakeIndex < 2u; ++lakeIndex)
    {
        const TerrainMesh::LakeRegion& lake = lakes[lakeIndex];
        const glm::vec4 sourceBounds = lake.boundsXZ;
        const float expansion = 24.0f + lake.maximumDepth * 1.65f;
        causticBounds[lakeIndex] = glm::vec4(
            sourceBounds.x - expansion, sourceBounds.y - expansion,
            sourceBounds.z + expansion, sourceBounds.w + expansion);
        for (int z = 0; z < PhotonGridResolution; ++z)
        {
            const float v = (static_cast<float>(z) + 0.5f) /
                            static_cast<float>(PhotonGridResolution);
            for (int x = 0; x < PhotonGridResolution; ++x)
            {
                const float u = (static_cast<float>(x) + 0.5f) /
                                static_cast<float>(PhotonGridResolution);
                photons.push_back({
                    glm::vec2(glm::mix(sourceBounds.x, sourceBounds.z, u),
                              glm::mix(sourceBounds.y, sourceBounds.w, v)),
                    static_cast<float>(lakeIndex)});
            }
        }
    }
    photonVertexCount = static_cast<int>(photons.size());

    glGenVertexArrays(1, &photonVAO);
    glGenBuffers(1, &photonVBO);
    glBindVertexArray(photonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, photonVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(photons.size() * sizeof(CausticPhotonVertex)),
                 photons.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          sizeof(CausticPhotonVertex),
                          reinterpret_cast<void*>(offsetof(CausticPhotonVertex, worldXZ)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE,
                          sizeof(CausticPhotonVertex),
                          reinterpret_cast<void*>(offsetof(CausticPhotonVertex, lakeIndex)));
    glBindVertexArray(0);

    glGenFramebuffers(static_cast<GLsizei>(causticFramebuffers.size()),
                      causticFramebuffers.data());
    glGenTextures(static_cast<GLsizei>(causticTextures.size()),
                  causticTextures.data());
    bool complete = true;
    for (std::size_t i = 0; i < causticTextures.size(); ++i)
    {
        glBindTexture(GL_TEXTURE_2D, causticTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F,
                     CausticAtlasWidth, CausticAtlasHeight, 0,
                     GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, causticFramebuffers[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, causticTextures[i], 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        complete = complete &&
            glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if (!complete)
    {
        std::cerr << "Water caustic framebuffer is incomplete" << std::endl;
        glDeleteTextures(static_cast<GLsizei>(causticTextures.size()),
                         causticTextures.data());
        glDeleteFramebuffers(static_cast<GLsizei>(causticFramebuffers.size()),
                             causticFramebuffers.data());
        glDeleteBuffers(1, &photonVBO);
        glDeleteVertexArrays(1, &photonVAO);
        causticTextures.fill(0u);
        causticFramebuffers.fill(0u);
        photonVBO = 0u;
        photonVAO = 0u;
        photonVertexCount = 0;
        return;
    }
    causticResourcesReady = true;
}

void LightingPass::renderCausticMap(Screenquad& screenQuad)
{
    initializeCausticResources();
    if (!causticResourcesReady || !resources.shaderLibrary ||
        !config.enableWater || !config.water.enableCaustics ||
        !config.enableDirectionalLight)
        return;

    GLint previousFramebuffer = 0;
    GLint previousViewport[4] = {0, 0, 0, 0};
    GLint previousBlendSourceRGB = GL_ONE;
    GLint previousBlendDestinationRGB = GL_ZERO;
    GLint previousBlendSourceAlpha = GL_ONE;
    GLint previousBlendDestinationAlpha = GL_ZERO;
    GLfloat previousClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_BLEND_SRC_RGB, &previousBlendSourceRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &previousBlendDestinationRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSourceAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDestinationAlpha);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean pointSizeWasEnabled = glIsEnabled(GL_PROGRAM_POINT_SIZE);

    glViewport(0, 0, CausticAtlasWidth, CausticAtlasHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBindFramebuffer(GL_FRAMEBUFFER, causticFramebuffers[0]);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Shader& photonShader = resources.shaderLibrary->waterCausticPhotons;
    photonShader.use();
    const WaterRenderSettings& water = config.water;
    photonShader.setFloat("terrainSize", resources.waterMesh->getTerrainSize());
    photonShader.setFloat("terrainBaseHeight",
                          resources.waterMesh->getTerrainBaseHeight());
    photonShader.setFloat("terrainMountainHeight",
                          resources.waterMesh->getTerrainMountainHeight());
    photonShader.setVec3("sunDirection", lightSettings.sunDirection);
    photonShader.setVec2("waterWindDirection", water.windDirection);
    photonShader.setFloat("waterTime", currentWaterTime);
    photonShader.setFloat("waterWaveAmplitude", water.waveAmplitude);
    photonShader.setFloat("waterWavelengthScale", water.wavelengthScale);
    photonShader.setFloat("opticalDisplacementScale",
                          std::max(water.causticCurvatureScale * 5.0f, 0.2f));
    photonShader.setVec4("causticBounds0", causticBounds[0]);
    photonShader.setVec4("causticBounds1", causticBounds[1]);
    photonShader.setFloat("photonPointSize",
                          glm::clamp(2.0f + water.causticScale * 8.0f,
                                     2.2f, 4.0f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, resources.waterMesh->getTerrainDataTexture());
    photonShader.setInt("terrainDataMap", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, resources.waterMesh->getLakeDataTexture());
    photonShader.setInt("lakeDataMap", 1);
    glBindVertexArray(photonVAO);
    photonShader.setBool("refractPhotons", true);
    photonShader.setInt("densityChannel", 0);
    glDrawArrays(GL_POINTS, 0, photonVertexCount);
    photonShader.setBool("refractPhotons", false);
    photonShader.setInt("densityChannel", 1);
    glDrawArrays(GL_POINTS, 0, photonVertexCount);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    Shader& blurShader = resources.shaderLibrary->waterCausticBlur;
    blurShader.use();
    blurShader.setInt("sourceDensity", 0);
    glBindFramebuffer(GL_FRAMEBUFFER, causticFramebuffers[1]);
    glClear(GL_COLOR_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, causticTextures[0]);
    blurShader.setVec2("blurDirection",
                       glm::vec2(1.15f / CausticAtlasWidth, 0.0f));
    screenQuad.drawTriangle();

    glBindFramebuffer(GL_FRAMEBUFFER, causticFramebuffers[0]);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, causticTextures[1]);
    blurShader.setVec2("blurDirection",
                       glm::vec2(0.0f, 1.15f / CausticAtlasHeight));
    screenQuad.drawTriangle();

    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1],
               previousViewport[2], previousViewport[3]);
    glClearColor(previousClearColor[0], previousClearColor[1],
                 previousClearColor[2], previousClearColor[3]);
    glBlendFuncSeparate(previousBlendSourceRGB, previousBlendDestinationRGB,
                        previousBlendSourceAlpha, previousBlendDestinationAlpha);
    if (blendWasEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (cullWasEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (pointSizeWasEnabled) glEnable(GL_PROGRAM_POINT_SIZE);
    else glDisable(GL_PROGRAM_POINT_SIZE);
}
