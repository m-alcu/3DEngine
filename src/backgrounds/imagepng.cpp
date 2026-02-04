#include <iostream>
#include "imagepng.hpp"
#include "../vendor/stb/stb_image.h"

void Imagepng::draw(uint32_t *pixels, uint16_t high_in, uint16_t width_in) {

    if (!getNeedsUpdate()) {
        return; // No need to update if not required
    }

    int img_width, img_height, channels;
    unsigned char* image = stbi_load("resources/PCwKbU.png", &img_width, &img_height, &channels, 4);

    if (!image) {
        std::cout << "Failed to load image: " << stbi_failure_reason() << std::endl;
        return;
    }

    // Now we have image (RGBA 8bit data), img_width, img_height
    // Fill the destination buffer
    for (uint16_t y = 0; y < high_in; ++y) {
        for (uint16_t x = 0; x < width_in; ++x) {

            // Repeat or clip the image as required
            unsigned src_x = (x < static_cast<unsigned>(img_width)) ? x : (x % img_width);
            unsigned src_y = (y < static_cast<unsigned>(img_height)) ? y : (y % img_height);

            // Index into the source image (RGBA, so 4 bytes per pixel)
            size_t src_index = (src_y * img_width + src_x) * 4;

            uint8_t r = image[src_index + 0];
            uint8_t g = image[src_index + 1];
            uint8_t b = image[src_index + 2];
            uint8_t a = image[src_index + 3];

            // You can choose any pixel format. Let's use ARGB here.
            pixels[y * width_in + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }

    stbi_image_free(image);
    setNeedsUpdate(false);

}
