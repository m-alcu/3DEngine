#pragma once

#include <render3d/scene.hpp>

#include <map>
#include <memory>


using namespace render3d;

struct AppState {
    std::unique_ptr<Scene> scene;
    std::map<int, bool> keys;
    bool closedWindow = false;
    int currentSceneIndex = 0;
};
