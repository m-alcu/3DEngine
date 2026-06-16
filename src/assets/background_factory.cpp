#include "background_factory.hpp"

#include <cstdio>
#include <utility>
#include <vector>

#include <render3d/backgrounds/desert.hpp>
#include <render3d/backgrounds/image_png.hpp>
#include <render3d/backgrounds/twister.hpp>
#include <render3d/backgrounds/skybox.hpp>
#include <render3d/backgrounds/hdr_panorama.hpp>
#include "texture_loader.hpp"
#include "cubemap_loader.hpp"

#include "../vendor/nothings/stb_image.h"


using namespace render3d;

namespace BackgroundFactory {

std::unique_ptr<Background> createSkybox(const std::string& px, const std::string& nx,
                                         const std::string& py, const std::string& ny,
                                         const std::string& pz, const std::string& nz) {
    return std::make_unique<Skybox>(CubeMapLoader::load(px, nx, py, ny, pz, nz));
}

std::unique_ptr<Background> createHdrPanorama(const std::string& path) {
    int width = 0, height = 0, channels = 0;
    float* data = stbi_loadf(path.c_str(), &width, &height, &channels, 3);
    if (!data) {
        std::fprintf(stderr, "HdrPanorama: failed to load '%s': %s\n",
                     path.c_str(), stbi_failure_reason());
        return std::make_unique<HdrPanorama>();
    }
    std::vector<float> pixels(data, data + static_cast<size_t>(width) * height * 3);
    stbi_image_free(data);
    return std::make_unique<HdrPanorama>(width, height, std::move(pixels));
}

std::unique_ptr<Background> create(BackgroundType type) {
    switch (type) {
        case BackgroundType::DESERT:
            return std::make_unique<Desert>();
        case BackgroundType::IMAGE_PNG:
            return std::make_unique<Imagepng>(
                TextureLoader::load("resources/PCwKbU.png"));
        case BackgroundType::TWISTER:
            return std::make_unique<Twister>(
                TextureLoader::load("resources/Honey2_Light.png"),
                TextureLoader::load("resources/Honey2_Dark.png"));
        case BackgroundType::SKYBOX:
            return createSkybox(
                "resources/skybox/1/px.png", "resources/skybox/1/nx.png",
                "resources/skybox/1/py.png", "resources/skybox/1/ny.png",
                "resources/skybox/1/pz.png", "resources/skybox/1/nz.png");
        case BackgroundType::HDR_PANORAMA:
            return createHdrPanorama("resources/hdrs/HDR_artificial_planet.hdr");
        default:
            return nullptr;
    }
}

} // namespace BackgroundFactory
