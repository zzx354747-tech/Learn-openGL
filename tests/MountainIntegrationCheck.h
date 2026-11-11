#pragma once
#include "rendering/editor/EditorTransform.h"
#include "rendering/passes/forward/SceneObjectPass.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <cmath>

// Opt-in real-GL check; all save/load operations use an isolated temporary directory.
inline bool runMountainIntegrationCheck(RendererScene& scene)
{
    const auto directory = std::filesystem::temp_directory_path() /
        ("mountain-integration-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove_all(path, e); } } cleanup{directory};
    auto require = [](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); };
    auto readJson = [](const auto& path) { std::ifstream f(path); nlohmann::json j; f >> j; return j; };
    auto image = [](GLuint fbo) {
        std::vector<float> pixels(320 * 240 * 4);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, 320, 240, GL_RGBA, GL_FLOAT, pixels.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        return pixels;
    };
    auto primitives = [](const std::function<void()>& draw) {
        GLuint query = 0, result = 0;
        glGenQueries(1, &query);
        glBeginQuery(GL_PRIMITIVES_GENERATED, query);
        draw();
        glEndQuery(GL_PRIMITIVES_GENERATED);
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);
        glDeleteQueries(1, &query);
        return result;
    };
    auto bindings = []() {
        GLint active = 0; glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        std::array<GLint, 10> result{};
        for (int unit = 0; unit < 5; ++unit)
        {
            glActiveTexture(GL_TEXTURE0 + unit);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &result[unit * 2]);
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &result[unit * 2 + 1]);
        }
        glActiveTexture(active);
        return result;
    };
    try
    {
        SceneObjectID id = InvalidSceneObjectID;
        for (const auto& object : scene.sceneObjectManager.getSceneObjects())
            if (std::holds_alternative<MountainDrawer*>(object.ref)) id = object.id;
        require(id != InvalidSceneObjectID, "Mountain object missing.");

        // Capture vertex inputs on the GPU: original grid and UVs must stay intact,
        // and locations 4/5 must contain a normalized UV-derived basis.
        const char* vertexSource = R"(#version 330 core
layout(location=0) in vec3 p; layout(location=2) in vec2 uv;
layout(location=4) in vec3 t; layout(location=5) in vec3 b;
out vec3 sourcePosition; out vec2 sourceUV;
out vec3 sourceTangent; out vec3 sourceBitangent;
void main() { sourcePosition=p; sourceUV=uv; sourceTangent=t; sourceBitangent=b; gl_Position=vec4(p,1); }
)";
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexSource, nullptr); glCompileShader(vertex);
        GLuint program = glCreateProgram(); glAttachShader(program, vertex);
        const char* varyings[] = {"sourcePosition", "sourceUV", "sourceTangent", "sourceBitangent"};
        glTransformFeedbackVaryings(program, 4, varyings, GL_INTERLEAVED_ATTRIBS);
        glLinkProgram(program);
        GLint linked = 0; glGetProgramiv(program, GL_LINK_STATUS, &linked);
        require(linked == GL_TRUE, "Vertex probe shader failed to link.");
        constexpr GLuint triangleCount = 99 * 99 * 2;
        constexpr GLsizeiptr feedbackBytes = triangleCount * 3 * 11 * sizeof(float);
        GLuint feedback = 0; glGenBuffers(1, &feedback);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedback);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, feedbackBytes, nullptr, GL_STREAM_READ);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedback);
        glUseProgram(program); glEnable(GL_RASTERIZER_DISCARD);
        glBeginTransformFeedback(GL_TRIANGLES); scene.mountainMesh.draw(); glEndTransformFeedback();
        glDisable(GL_RASTERIZER_DISCARD);
        std::vector<float> vertices(feedbackBytes / sizeof(float));
        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedbackBytes, vertices.data());
        float maxX = 0, maxZ = 0;
        for (std::size_t offset = 0; offset < vertices.size(); offset += 11)
        {
            const glm::vec3 p(vertices[offset], vertices[offset + 1], vertices[offset + 2]);
            const glm::vec2 uv(vertices[offset + 3], vertices[offset + 4]);
            const glm::vec3 t(vertices[offset + 5], vertices[offset + 6], vertices[offset + 7]);
            const glm::vec3 b(vertices[offset + 8], vertices[offset + 9], vertices[offset + 10]);
            require(std::abs(p.y) < 1e-6f && glm::length(uv - glm::vec2(p.x, p.z) / 990.0f) < 1e-5f,
                    "Original grid positions or UVs changed.");
            require(glm::length(t - glm::vec3(1, 0, 0)) < 1e-5f && glm::length(b - glm::vec3(0, 0, 1)) < 1e-5f,
                    "Invalid tangent/bitangent vertex attributes.");
            maxX = std::max(maxX, p.x); maxZ = std::max(maxZ, p.z);
        }
        require(maxX == 990 && maxZ == 990, "Original mountain dimensions changed.");
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);
        glDeleteBuffers(1, &feedback); glUseProgram(0);
        glDeleteProgram(program); glDeleteShader(vertex);
        std::cout << "Mountain grid/UV/tangent GPU readback PASS\n";

        scene.sceneConfig.sceneSelection = SceneSelection::Mountain;
        scene.sceneConfig.enableEdit = true;
        scene.sceneConfig.enableSkybox = false;
        scene.sceneConfig.enablePointLight = false;
        scene.sceneConfig.enableDirectionalLight = true;
        scene.sceneConfig.enableFlashlight = false;
        scene.sceneConfig.enableBloom = false;
        scene.camera.setCameraState(glm::vec3(0, 5, 4), glm::vec3(0, 1, 0), -90, -30, 2.5f, 0.1f);
        auto* mountain = scene.sceneObjectManager.getSceneObject(id);
        // Fit only this test instance to the existing camera/shadow range.
        // The production mesh, initial transform and generation parameters are unchanged.
        mountain->transform.position = glm::vec3(-7, -1, -12);
        mountain->transform.scale = glm::vec3(0.015f);
        require(scene.selectObjectAtPixel(160, 120, 320, 240) == id, "Center picking failed.");
        mountain->isVisible = false;
        require(scene.selectObjectAtPixel(160, 120, 320, 240) == InvalidSceneObjectID, "Hidden mountain remained pickable.");
        mountain->isVisible = true;

        // Only the Mountain draw entry should submit its triangles in GeometryPass.
        require(primitives([&] { scene.geometryPass.render(320, 240); }) == triangleCount,
                "GeometryPass omitted or duplicated the Mountain draw.");
        auto& geometry = scene.shaderLibrary.geometryPBR;
        GLint normalMap = 1, parallaxMap = 1;
        glGetUniformiv(geometry.ID, glGetUniformLocation(geometry.ID, "hasNormalMap"), &normalMap);
        glGetUniformiv(geometry.ID, glGetUniformLocation(geometry.ID, "hasParallaxMap"), &parallaxMap);
        require(normalMap == 1 && parallaxMap == 1, "Mountain material maps were not restored.");
        const auto beforeDepth = bindings();
        require(primitives([&] { scene.directionalShadowPass.render(scene.sceneObjectManager, SceneSelection::Mountain); }) == triangleCount,
                "Directional shadow draw count differs.");
        require(bindings() == beforeDepth, "Directional shadow bound material textures.");
        require(primitives([&] { scene.pointShadowPass.render(scene.sceneState.lightPositions, scene.sceneObjectManager, SceneSelection::Mountain); }) == triangleCount * 6,
                "Point shadow draw count differs.");
        require(bindings() == beforeDepth, "Point shadow bound material textures.");
        require(primitives([&] { scene.spotShadowPass.render(scene.sceneObjectManager, SceneSelection::Mountain); }) == triangleCount,
                "Spot shadow draw count differs.");
        require(bindings() == beforeDepth, "Spot shadow bound material textures.");
        // Verify actual depth was written, not just submitted.
        auto depthWritten = [](GLuint texture, GLenum target, int width, int height) {
            std::vector<float> depth(width * height);
            glBindTexture(target == GL_TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP, texture);
            glGetTexImage(target, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
            return std::any_of(depth.begin(), depth.end(), [](float d) { return std::isfinite(d) && d < 0.9999f; });
        };
        require(depthWritten(scene.shadowMap.getDepthMapTexture(), GL_TEXTURE_2D, scene.shadowMap.getWidth(), scene.shadowMap.getHeight()),
                "Directional shadow contains no Mountain depth.");
        require(depthWritten(scene.spotShadowMap.getDepthMap(), GL_TEXTURE_2D, scene.spotShadowMap.getWidth(), scene.spotShadowMap.getHeight()),
                "Spot shadow contains no Mountain depth.");
        bool cubeDepth = false;
        for (int face = 0; face < 6; ++face)
            cubeDepth |= depthWritten(scene.pointShadowMap.getDepthCubeMap(), GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                      scene.pointShadowMap.getWidth(), scene.pointShadowMap.getHeight());
        require(cubeDepth, "Point shadow contains no Mountain depth.");
        std::cout << "Material single submission and three shadow depth maps PASS\n";

        for (int mode = 0; mode < 3; ++mode)
        {
            scene.sceneConfig.renderMode = mode == 0 ? RenderMode::Lighting : (mode == 1 ? RenderMode::Basic : RenderMode::Reflection);
            scene.sceneConfig.forwardLightMode = mode == 0 ? ForwardLightMode::Light : (mode == 1 ? ForwardLightMode::Basic : ForwardLightMode::Reflect);
            scene.selectedObjectID = InvalidSceneObjectID;
            if (mode != 0)
            {
                scene.fb.bind();
                require(primitives([&] {
                    SceneObjectPass::renderSceneObjectPass(scene.camera, scene.sceneResources, scene.sceneConfig,
                        scene.sphereDrawer, scene.livingRoomDrawer, scene.sceneObjectManager, 320, 240);
                }) == triangleCount, "Forward mode omitted or duplicated Mountain.");
                scene.fb.unbind();
            }
            scene.render(320, 240); glFinish();
            const auto pixels = image(scene.fb.getFBO());
            const auto offset = (120 * 320 + 160) * 4;
            require(std::isfinite(pixels[offset]) && std::isfinite(pixels[offset + 1]) && std::isfinite(pixels[offset + 2]) && pixels[offset + 3] > 0.5f,
                    "Render mode did not produce a finite opaque Mountain pixel.");
            require(glGetError() == GL_NO_ERROR, "GL error in render mode.");
            std::cout << "Mountain render mode " << mode << " PASS\n";
        }

        scene.sceneConfig.renderMode = RenderMode::Lighting;
        scene.sceneConfig.forwardLightMode = ForwardLightMode::Light;
        scene.render(320, 240);
        const auto plain = image(scene.fb.getFBO());
        scene.selectedObjectID = id;
        scene.render(320, 240);
        const auto outlined = image(scene.fb.getFBO());
        unsigned changed = 0;
        for (std::size_t i = 0; i < plain.size(); i += 4)
            if (std::abs(plain[i] - outlined[i]) > 0.01f || std::abs(plain[i + 1] - outlined[i + 1]) > 0.01f) ++changed;
        require(changed > 20, "Selected Mountain has no visible outline pixels.");
        // Force nearer depth everywhere: the depth-tested mask must disappear.
        scene.sceneGBuffer.bind();
        const float nearDepth = 0; glClearBufferfv(GL_DEPTH, 0, &nearDepth);
        scene.sceneGBuffer.unbind(); scene.fb.bind();
        const auto occludedBefore = image(scene.fb.getFBO());
        scene.outlinePass.render(mountain, 320, 240);
        require(image(scene.fb.getFBO()) == occludedBefore, "Outline ignored nearer occluding depth.");
        scene.objectPickPass.begin();
        glClearBufferfv(GL_DEPTH, 0, &nearDepth);
        auto& pick = scene.shaderLibrary.objectPick;
        pick.use(); CameraUniformSetter::apply(pick, scene.camera, 320, 240);
        pick.setUInt("objectID", id);
        scene.mountainDrawer.drawDepth(pick, mountain->transform.matrix());
        require(scene.objectPickPass.readID(160, 120) == InvalidSceneObjectID, "Picking ignored nearer occluding depth.");
        scene.objectPickPass.end();
        std::cout << "Picking, outline pixels and depth occlusion PASS (" << changed << " outline pixels)\n";

        Transform edited = mountain->transform;
        edited.position.y += 0.3f;
        edited.rotationDegrees = glm::vec3(0, 8, 0);
        edited.scale = glm::vec3(0.014f, 0.015f, 0.012f);
        require(EditorTransform::applyMatrix(edited.matrix(), mountain->transform), "Gizmo matrix decomposition rejected Mountain.");
        for (int c = 0; c < 4; ++c)
            require(glm::length(edited.matrix()[c] - mountain->transform.matrix()[c]) < 1e-4f, "Gizmo Transform round trip differs.");
        require(scene.selectObjectAtPixel(160, 120, 320, 240) == id, "Edited Mountain no longer pickable at expected surface.");
        scene.render(320, 240);
        const auto save = directory / "scene.json", after = directory / "after.json";
        require(scene.saveSceneToFile(save), "Mountain save failed.");
        const auto expected = readJson(save);
        mountain->isVisible = false; mountain->transform.position.y = -40;
        for (int repeat = 0; repeat < 2; ++repeat)
        {
            require(scene.loadSceneFromFile(save), "Mountain restore failed.");
            require(scene.selectObjectAtPixel(160, 120, 320, 240) == id, "Restored Mountain pick failed.");
            require(scene.saveSceneToFile(after) && readJson(after) == expected, "Full scene round trip changed state or duplicated objects.");
            scene.render(320, 240);
        }
        auto hidden = expected;
        for (auto& object : hidden["objects"])
            if (object["type"] == "mountain") { object["isVisible"] = false; object["scene"] = static_cast<int>(SceneSelection::Default); }
        { std::ofstream f(after); f << hidden; }
        require(scene.loadSceneFromFile(after), "Hidden Mountain in another scene failed to restore.");
        require(scene.saveSceneToFile(after) && readJson(after) == hidden, "Visibility or scene membership was not preserved.");
        require(scene.loadSceneFromFile(save), "Could not restore visible Mountain.");
        require(glGetError() == GL_NO_ERROR, "GL error after Mountain restore.");
        std::cout << "Gizmo matrix data path and full-state repeated save/load PASS\n";
        std::cout << "MOUNTAIN_INTEGRATION_CHECK PASS (GUI mouse dragging is not exercised by this check).\n";
        return true;
    }
    catch (const std::exception& error)
    {
        std::cerr << "MOUNTAIN_INTEGRATION_CHECK FAIL: " << error.what() << '\n';
        return false;
    }
}
