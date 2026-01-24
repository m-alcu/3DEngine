#pragma once

#include <memory>
#include "background.hpp"
#include "desert.hpp"
#include "imagepng.hpp"
#include "twister.hpp"

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