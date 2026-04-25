#pragma once

#include "constants.hpp"
#include "scene.hpp"

#include <map>
#include <memory>

struct AppState {
    std::unique_ptr<Scene> scene;
    std::map<int, bool> keys;
    bool closedWindow = false;
    int currentSceneIndex = 0;
    Screen screen{SCREEN_WIDTH, SCREEN_HEIGHT};
};
