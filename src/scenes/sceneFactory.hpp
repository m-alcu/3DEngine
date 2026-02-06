#pragma once

#include <memory>
#include <string>
#include <vector>
#include "../scene.hpp"

class SceneFactory {
public:
    static std::unique_ptr<Scene> createScene(SceneType type, Screen scr);
    static std::unique_ptr<Scene> createSceneFromYaml(const std::string& yamlPath,
                                                       Screen scr);

    // Scan a directory for .yaml scene files and register them
    static void scanYamlScenes(const std::string& directory);

    // Create a scene by combined index (built-in scenes first, then YAML scenes)
    static std::unique_ptr<Scene> createSceneByIndex(int index, Screen scr);

    // Get the combined list of scene names (built-in + YAML)
    static const std::vector<std::string>& allSceneNames();

    // Total number of scenes (built-in + YAML)
    static int sceneCount();

private:
    static std::vector<std::string> yamlPaths_;
    static std::vector<std::string> yamlNames_;
    static std::vector<std::string> combinedNames_;
    static bool scanned_;

    static void buildCombinedNames();
};
