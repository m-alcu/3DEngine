#pragma once

#include <memory>
#include "Background.hpp"
#include "Desert.hpp"
#include "Imagepng.hpp"
#include "Twister.hpp"

enum class BackgroundType {
    DESERT,
    IMAGE_PNG,
    TWISTER
};

static const char* backgroundNames[] = {
    "Desert",
    "Image PNG",
    "Twister"
};

class BackgroundFactory {
public:
    static std::unique_ptr<Background> createBackground(BackgroundType type);
};