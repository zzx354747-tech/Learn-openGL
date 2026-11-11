#pragma once
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

// Same terrain patch and relative camera, translated together: AO should not change.
// Reallocate only gPosition to isolate world-position storage precision on the GPU.
inline bool runSSAOPrecisionCheck(RendererScene& scene, const std::filesystem::path& directory)
{
    constexpr int width = 640, height = 480;
    struct Capture
    {
        std::vector<float> position, normal, depth, raw, blurred, lighting;
    };
    auto require = [](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); };
    auto texture = [](GLuint id, GLenum format, int channels) {
        std::vector<float> data(width * height * channels);
        glBindTexture(GL_TEXTURE_2D, id);
        glGetTexImage(GL_TEXTURE_2D, 0, format, GL_FLOAT, data.data());
        return data;
    };
    auto ppm = [&](const std::string& name, const std::vector<float>& data, int channels, bool hdr = false) {
        std::ofstream out(directory / (name + ".ppm"), std::ios::binary);
        out << "P6\n" << width << ' ' << height << "\n255\n";
        for (int y = height - 1; y >= 0; --y)
            for (int x = 0; x < width; ++x)
                for (int c = 0; c < 3; ++c)
                {
                    float value = data[(y * width + x) * channels + (channels == 1 ? 0 : c)];
                    if (hdr) value = std::pow(1.0f - std::exp(-std::max(0.0f, value)), 1.0f / 2.2f);
                    const unsigned char byte = static_cast<unsigned char>(std::clamp(value, 0.0f, 1.0f) * 255.0f);
                    out.write(reinterpret_cast<const char*>(&byte), 1);
                }
        require(static_cast<bool>(out), "Could not write SSAO diagnostic image.");
    };
    try
    {
        std::filesystem::create_directories(directory);
        GLint initialFormat = 0, resizedFormat = 0;
        glBindTexture(GL_TEXTURE_2D, scene.sceneGBuffer.getPositionTexture());
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &initialFormat);
        scene.resize(width, height);
        glBindTexture(GL_TEXTURE_2D, scene.sceneGBuffer.getPositionTexture());
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &resizedFormat);
        require(initialFormat == GL_RGB32F && resizedFormat == GL_RGB32F,
            "World positions must retain 32-bit precision after initialization and resize.");
        scene.sceneConfig.sceneSelection = SceneSelection::Mountain;
        scene.sceneConfig.renderMode = RenderMode::Lighting;
        scene.sceneConfig.enableSSAO = true;
        scene.sceneConfig.enableEdit = false;
        scene.sceneConfig.enableBloom = false;
        // Isolate AO in the final composition too: uniform ambient illumination.
        scene.sceneConfig.enableIBL = false;
        scene.sceneConfig.fixedAmbientColor = glm::vec3(1.0f);
        scene.sceneConfig.fixedAmbientStrength = 1.0f;
        scene.sceneConfig.enablePointLight = false;
        scene.sceneConfig.enableDirectionalLight = false;
        scene.sceneConfig.enableFlashlight = false;
        scene.renderParams.enableNormalMapping = false;
        scene.renderParams.enableParallaxMapping = false;
        SceneObject* mountain = nullptr;
        for (const auto& object : scene.sceneObjectManager.getSceneObjects())
            if (std::holds_alternative<MountainDrawer*>(object.ref))
                mountain = scene.sceneObjectManager.getSceneObject(object.id);
        require(mountain != nullptr, "Mountain is missing.");
        auto capture = [&](GLint format, float translation, const std::string& name) {
            glBindTexture(GL_TEXTURE_2D, scene.sceneGBuffer.getPositionTexture());
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
            scene.sceneGBuffer.bind();
            require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Position format is not renderable.");
            scene.sceneGBuffer.unbind();
            mountain->transform.position = glm::vec3(-768.0f + translation, -1.0f, -768.0f + translation);
            mountain->transform.rotationDegrees = glm::vec3(0.0f);
            mountain->transform.scale = glm::vec3(1.0f);
            mountain->isVisible = true;
            scene.camera.setCameraState(glm::vec3(translation, 44.0f, translation + 22.0f),
                glm::vec3(0, 1, 0), -90.0f, -35.0f, 2.5f, 0.1f);
            scene.geometryPass.render(width, height);
            scene.ssaoCommonPass.render(width, height);
            scene.lightingPass.render(scene.fb, scene.screenQuad);
            glFinish();
            Capture result;
            result.position = texture(scene.sceneGBuffer.getPositionTexture(), GL_RGB, 3);
            result.normal = texture(scene.sceneGBuffer.getNormalRoughnessTexture(), GL_RGBA, 4);
            result.raw = texture(scene.sceneSSAO.getSSAOColorBuffer(), GL_RED, 1);
            result.blurred = texture(scene.sceneSSAO.getBilateralBlurTexture(), GL_RED, 1);
            result.lighting = texture(scene.fb.getTextureID(0), GL_RGB, 3);
            result.depth.resize(width * height);
            scene.sceneGBuffer.bind();
            glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, result.depth.data());
            scene.sceneGBuffer.unbind();
            std::vector<float> positions(width * height * 3), normals(width * height * 3), depths(width * height);
            for (int i = 0; i < width * height; ++i)
            {
                const glm::vec3 p(result.position[i * 3], result.position[i * 3 + 1], result.position[i * 3 + 2]);
                const glm::vec3 relative = p - glm::vec3(translation, 0.0f, translation);
                for (int c = 0; c < 3; ++c)
                {
                    positions[i * 3 + c] = relative[c] * 0.025f + 0.5f;
                    normals[i * 3 + c] = result.normal[i * 4 + c] * 0.5f + 0.5f;
                }
                const float z = result.depth[i] * 2.0f - 1.0f;
                depths[i] = (2.0f * 0.1f * Camera::FarPlane) /
                    (Camera::FarPlane + 0.1f - z * (Camera::FarPlane - 0.1f)) / 100.0f;
            }
            ppm(name + "-position", positions, 3);
            ppm(name + "-normal", normals, 3);
            ppm(name + "-depth", depths, 1);
            ppm(name + "-raw", result.raw, 1);
            ppm(name + "-blurred", result.blurred, 1);
            ppm(name + "-lighting", result.lighting, 3, true);
            require(glGetError() == GL_NO_ERROR, "GL error in SSAO diagnostic capture.");
            return result;
        };
        const auto halfNear = capture(GL_RGB16F, 0.0f, "16f-near");
        const auto halfFar = capture(GL_RGB16F, 768.0f, "16f-far");
        const auto fullNear = capture(GL_RGB32F, 0.0f, "32f-near");
        const auto fullFar = capture(GL_RGB32F, 768.0f, "32f-far");
        auto difference = [&](const Capture& a, const Capture& b, bool blurred) {
            double sum = 0;
            int count = 0;
            for (int y = 16; y < height - 16; ++y)
                for (int x = 16; x < width - 16; ++x)
                {
                    const int i = y * width + x;
                    if (a.depth[i] >= 1.0f || b.depth[i] >= 1.0f) continue;
                    const float d = std::abs((blurred ? a.blurred[i] : a.raw[i]) - (blurred ? b.blurred[i] : b.raw[i]));
                    sum += d; ++count;
                }
            require(count > width * height / 4, "Diagnostic camera does not cover enough terrain.");
            return sum / count;
        };
        const double halfRaw = difference(halfNear, halfFar, false);
        const double fullRaw = difference(fullNear, fullFar, false);
        const double halfBlur = difference(halfNear, halfFar, true);
        const double fullBlur = difference(fullNear, fullFar, true);
        nlohmann::json report = {{"16f_translation_raw_mean_abs", halfRaw}, {"32f_translation_raw_mean_abs", fullRaw},
            {"16f_translation_blurred_mean_abs", halfBlur}, {"32f_translation_blurred_mean_abs", fullBlur}};
        std::ofstream(directory / "metrics.json") << report.dump(2) << '\n';
        std::cout << "SSAO_PRECISION_METRICS " << report.dump() << '\n';
        require(halfRaw > 0.02 && fullRaw < halfRaw * 0.2 && fullBlur < halfBlur * 0.2,
            "World-position precision did not explain translation-dependent SSAO artifacts.");
        // End-to-end production pipeline: SSAO must stay active for Mountain.
        scene.render(width, height);
        GLint ssaoEnabled = 0;
        glGetUniformiv(scene.shaderLibrary.lightingPass.ID,
            glGetUniformLocation(scene.shaderLibrary.lightingPass.ID, "enableSSAO"), &ssaoEnabled);
        require(ssaoEnabled == GL_TRUE, "Mountain SSAO is still bypassed in the lighting pass.");
        const auto productionRaw = texture(scene.sceneSSAO.getSSAOColorBuffer(), GL_RED, 1);
        require(productionRaw == fullFar.raw, "Production pipeline did not regenerate Mountain SSAO.");
        ppm("production-final", texture(scene.editorOutput.getTextureID(0), GL_RGB, 3), 3);
        require(glGetError() == GL_NO_ERROR, "GL error in the production SSAO pipeline.");
        std::cout << "SSAO_PRECISION_CHECK PASS: identical terrain and relative camera; only world offset and position format changed.\n";
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "SSAO_PRECISION_CHECK FAIL: " << error.what() << '\n';
        return false;
    }
}
