#pragma once

#include <render3d/constants.hpp>
#include <render3d/scene.hpp>


using namespace render3d;

struct AppConfig {
    const char* windowTitle = "3D Engine";
    Screen screen{SCREEN_WIDTH, SCREEN_HEIGHT};
    int windowScale = 2;
};
