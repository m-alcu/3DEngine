#include "background_factory.hpp"

std::unique_ptr<Background> BackgroundFactory::createBackground(BackgroundType type) {
    switch (type) {
        case BackgroundType::DESERT:
            return std::make_unique<Desert>();
        case BackgroundType::IMAGE_PNG:
            return std::make_unique<Imagepng>();
        case BackgroundType::TWISTER:
            return std::make_unique<Twister>();
        case BackgroundType::SKYBOX:
            return std::make_unique<Skybox>();
        case BackgroundType::HDR_PANORAMA:
            return std::make_unique<HdrPanorama>();
        default:
            return nullptr;
    }
}
