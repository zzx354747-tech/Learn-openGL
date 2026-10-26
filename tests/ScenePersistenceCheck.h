#pragma once
#include "scene/SceneSaver.h"
#include "scene/SceneLoder.h"
#include <chrono>

// Run through the same RendererScene entry points as File -> Save/Load, with a real GL context.
inline bool runScenePersistenceCheck(RendererScene& scene)
{
    const auto directory = std::filesystem::temp_directory_path() /
        ("opengl-scene-check-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove_all(path, e); } } cleanup{directory};
    const auto save = directory / "scene.json";
    const auto after = directory / "after.json";
    const auto model = directory / "triangle.obj";
    auto require = [](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); };
    auto read = [](const auto& path) { std::ifstream f(path); nlohmann::json j; f >> j; return j; };
    auto write = [](const auto& path, const nlohmann::json& j) { std::ofstream f(path); f << j.dump(2); };
    try
    {
        { std::ofstream f(model); f << "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1\n"; }
        auto id = scene.sceneObjectManager.importModel(model.string(), SceneSelection::LivingRoom, glm::vec3(1,2,3));
        auto* object = scene.sceneObjectManager.getSceneObject(id);
        object->name = "Saved imported triangle";
        object->transform.rotationDegrees = glm::vec3(15, 25, 35);
        object->transform.scale = glm::vec3(0.8f, 1.1f, 0.6f);
        scene.sceneObjectManager.getSceneObject(2)->isVisible = false;
        scene.sceneObjectManager.getSceneObject(1)->scene = SceneSelection::LivingRoom;
        scene.camera.setCameraState(glm::vec3(4,5,6), glm::vec3(0,1,0), -112, 23, 4.1f, 0.17f);
        scene.sceneConfig.exposure = 1.73f;
        scene.lightSettings.sunDirection = glm::vec3(-0.3f, -0.8f, 0.2f);
        scene.lightSettings.sunIntensity = 3.4f;
        scene.renderParams.numLayers = 41;
        scene.sceneState.lightPositions = glm::vec3(3,7,-4);
        scene.selectedObjectID = id;
        scene.sceneRenderUI.gizmoOperation = 1;
        scene.sceneRenderUI.gizmoSpace = 1;
        scene.sceneRenderUI.gizmoSnap = true;
        require(scene.saveSceneToFile(save), "Initial save failed.");
        auto expected = read(save);
        // Exercise every serialized scalar/vector, even fields not changed by the UI scenario above.
        for (const auto* section : {"config", "lighting", "surface"})
            for (auto& field : expected[section].items())
            {
                auto& value = field.value();
                if (value.is_boolean()) value = !value.get<bool>();
                else if (value.is_number_float()) value = value.get<float>() * 0.8f + 0.031f;
                else if (value.is_array()) for (auto& v : value) v = v.get<float>() * 0.7f + 0.02f;
            }
        expected["config"]["environmentSelection"] = static_cast<int>(EnvironmentSelection::Sunny);
        expected["config"]["renderMode"] = static_cast<int>(RenderMode::Lighting);
        expected["config"]["forwardLightMode"] = static_cast<int>(ForwardLightMode::Light);
        // Normalize through typed deserialization to float precision.
        expected = sceneToJson(sceneFromJson(expected));
        write(save, expected);
        scene.sceneConfig.exposure = 0.1f;
        scene.renderParams.numLayers = 2;
        scene.camera.ProcessKeyboard(FORWARD, 1);
        scene.sceneObjectManager.importModel(model.string(), SceneSelection::Default, glm::vec3(0));
        scene.queueModelImport(model.string());
        require(scene.loadSceneFromFile(save), "Restore failed.");
        require(scene.pendingModelImports.empty(), "Queued import survived restore.");
        require(scene.saveSceneToFile(after), "Restored save failed.");
        require(read(after) == expected, "Full state round trip differs.");
        require(scene.loadSceneFromFile(save), "Repeat restore failed.");
        require(scene.saveSceneToFile(after) && read(after) == expected, "Repeat restore duplicated objects or changed state.");
        // Model and environment resources survived repeated replacement.
        scene.render(320, 240);
        glFinish();
        require(glGetError() == GL_NO_ERROR, "GL error rendering the restored scene.");
        // Invalid files must leave all current state untouched.
        auto reject = [&](nlohmann::json bad) {
            write(directory / "invalid.json", bad);
            require(!scene.loadSceneFromFile(directory / "invalid.json"), "Invalid save was accepted.");
            require(scene.saveSceneToFile(after) && read(after) == expected, "Failed load changed renderer state.");
        };
        auto bad = expected; bad["camera"]["pitch"] = 95; reject(bad);
        bad = expected; bad["config"]["environmentSelection"] = 90; reject(bad);
        bad = expected; bad["objects"][1]["id"] = bad["objects"][0]["id"]; reject(bad);
        bad = expected; bad["lighting"].erase("sunIntensity"); reject(bad);
        bad = expected; bad["objects"].back()["modelPath"] = (directory / "missing.obj").string(); reject(bad);
        bad = expected; bad["formatVersion"] = 999; reject(bad);
        bad = expected; bad["objects"][0]["id"] = 4294967297ULL; reject(bad);
        bad = expected; bad["surface"]["numLayers"] = 1.5; reject(bad);
        bad = expected; bad["camera"]["position"] = {0, nullptr, 2}; reject(bad);
        // Force an environment read failure without changing any real asset files.
        bad = expected;
        bad["config"]["environmentSelection"] = static_cast<int>(EnvironmentSelection::Night);
        write(directory / "invalid.json", bad);
        {
            struct RestoreCwd { std::filesystem::path path; ~RestoreCwd() { std::filesystem::current_path(path); } } cwd{std::filesystem::current_path()};
            std::filesystem::current_path(directory);
            require(!scene.loadSceneFromFile(directory / "invalid.json"), "Missing environment was accepted.");
        }
        require(scene.saveSceneToFile(after) && read(after) == expected, "Environment failure changed state.");
        scene.render(320, 240);
        glFinish();
        require(glGetError() == GL_NO_ERROR, "Environment failure damaged existing GPU resources.");
        { std::ofstream f(directory / "invalid.json"); f << "{ broken"; }
        require(!scene.loadSceneFromFile(directory / "invalid.json"), "Malformed JSON accepted.");
        require(!scene.loadSceneFromFile(directory / "absent.json"), "Absent save accepted.");
        require(scene.saveSceneToFile(after) && read(after) == expected, "Read failure changed state.");
        // A failed save must not truncate the previous good file.
        auto invalid = sceneFromJson(expected);
        invalid.camera.pitch = 100;
        bool rejected = false;
        try { saveScene(invalid, save); } catch (const std::exception&) { rejected = true; }
        require(rejected, "Invalid save accepted.");
        require(read(save) == expected, "Failed save damaged the previous file.");
        std::filesystem::create_directory(save.string() + ".tmp");
        require(!scene.saveSceneToFile(save), "Unwritable temporary file was accepted.");
        require(read(save) == expected, "Write failure damaged the previous file.");
        // Independent slots: later saves must not overwrite another environment.
        std::vector<std::filesystem::path> slots;
        for (const auto environment : {EnvironmentSelection::Night, EnvironmentSelection::Sunny, EnvironmentSelection::NightN8_3K})
        {
            auto document = sceneFromJson(expected);
            document.config.environmentSelection = environment;
            document.config.exposure = 0.5f + static_cast<int>(environment);
            const auto slot = sceneFilePath(environment, directory);
            require(std::find(slots.begin(), slots.end(), slot) == slots.end(), "Environment save paths collide.");
            require(saveScene(document, slot), "Environment save failed.");
            slots.push_back(slot);
        }
        for (unsigned i = 0; i < slots.size(); ++i)
        {
            require(scene.loadSceneFromFile(slots[i]), "Environment slot restore failed.");
            require(static_cast<unsigned>(scene.sceneConfig.environmentSelection) == i, "Wrong environment restored.");
            require(scene.sceneConfig.exposure == 0.5f + i, "Environment slots overwrote each other.");
        }
        require(scene.loadSceneFromFile(save), "Could not restore original test state.");
        // Legacy built-in saves remain loadable, but cannot invent missing imported sources.
        auto legacy = expected;
        legacy.erase("formatVersion"); legacy.erase("editor");
        legacy["objects"].erase(legacy["objects"].end() - 1);
        for (auto& o : legacy["objects"]) { o.erase("type"); o.erase("modelPath"); o.erase("materialIndex"); }
        sceneFromJson(legacy);
        const auto newID = scene.sceneObjectManager.importModel(model.string(), SceneSelection::Default, glm::vec3(0));
        require(newID > id, "Next object ID was not restored.");
        std::cout << "SCENE_PERSISTENCE_CHECK PASS: full round trip, imported assets, environment rebuild, repeat load, rendering, invalid input rollback, atomic save, independent environment slots, legacy format, next ID.\n";
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "SCENE_PERSISTENCE_CHECK FAIL: " << e.what() << '\n';
        return false;
    }
}
