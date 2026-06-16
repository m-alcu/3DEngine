#include "texture_loader.hpp"

#include <iostream>
#include <utility>
#include <vector>

#include "../vendor/nothings/stb_image.h"


using namespace render3d;

namespace TextureLoader {

Texture load(const std::string& filename, TextureFilter filter) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* imageData = stbi_load(filename.c_str(), &width, &height, &channels, 4);

    if (!imageData) {
        std::cout << "Failed to load image: " << filename
                  << " - " << stbi_failure_reason() << std::endl;
        return {};
    }

    std::vector<unsigned char> image(imageData, imageData + (width * height * 4));
    stbi_image_free(imageData);

    Texture texture{width, height, std::move(image)};
    texture.setFilter(filter);
    return texture;
}

} // namespace TextureLoader
