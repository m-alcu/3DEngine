#pragma once

#include <memory>
#include "../scene.hpp"

class SceneFactory {
public:
    static std::unique_ptr<Scene> createScene(SceneType type, Screen scr);
};