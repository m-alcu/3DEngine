#pragma once

#include <memory>
#include <string>
#include "../scene.hpp"

class SceneFactory {
public:
    static std::unique_ptr<Scene> createScene(SceneType type, Screen scr);
    static std::unique_ptr<Scene> createSceneFromYaml(const std::string& yamlPath,
                                                       Screen scr);
};