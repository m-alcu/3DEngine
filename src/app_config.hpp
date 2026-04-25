#pragma once

#include "constants.hpp"
#include "scene.hpp"

struct AppConfig {
    const char* windowTitle = "3D Engine";
    Screen screen{SCREEN_WIDTH, SCREEN_HEIGHT};
    int windowScale = 2;
};
