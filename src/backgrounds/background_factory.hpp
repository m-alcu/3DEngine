#pragma once

#include <memory>
#include "background.hpp"
#include "desert.hpp"
#include "image_png.hpp"
#include "twister.hpp"
#include "skybox.hpp"
#include "hdr_panorama.hpp"

enum class BackgroundType {
    DESERT,
    IMAGE_PNG,
    TWISTER,
    SKYBOX,
    HDR_PANORAMA
};

static const char* backgroundNames[] = {
    "Desert",
    "Image PNG",
    "Twister",
    "Skybox",
    "HDR Panorama"
};

class BackgroundFactory {
public:
    static std::unique_ptr<Background> createBackground(BackgroundType type);
};