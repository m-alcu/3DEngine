#include <iostream>
#include <algorithm>
#include "twister.hpp"


slib::texture Twister::DecodePng(const char* filename)
{
    std::vector<unsigned char> buffer;
    std::vector<unsigned char> image; // the raw pixels
    lodepng::load_file(buffer, filename);
    unsigned width, height;

    lodepng::State state;

    // decode
    unsigned error = lodepng::decode(image, width, height, state, buffer);
    const LodePNGColorMode& color = state.info_png.color;
    auto bpp = lodepng_get_bpp(&color);
    // if there's an error, display it
    if (error)
    {
        std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        exit(1);
    }

    // the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw
    // it, ...
    return {static_cast<int>(width), static_cast<int>(height), image, 4};
}


void Twister::texLine(uint32_t* pixels, int pitch, int x1, int x2, int v, int l, const slib::texture& tex, int width) {
    int dx = std::abs(x2 - x1);
    if (dx == 0) return;

    const int repeats = 1; // Change to make the bands narrower or wider

    for (int x = x1; x < x2 && x < width; ++x) {
        float tx = float(x - x1) / dx;
        int cx = int(tx * tex.w * repeats) % tex.w;
        int cy = v % tex.h;
        int index = 4 * (cy * tex.w + cx);

        float scale = (255.0f - l) / 255.0f;
        float r = tex.data[index + 0] * scale;
        float g = tex.data[index + 1] * scale;
        float b = tex.data[index + 2] * scale;

        pixels[x + width * v] = Color(r, g, b).toBgra(); // Assuming Color has a toBgra() method

    }
}

void Twister::rasterScan(uint32_t* pixels, int pitch, int v, float* x, const slib::texture& tex, int width) {
    float lum[4];
    for (int i = 0; i < 4; ++i) {
        float len = 0.25f + std::abs(x[i] - x[(i + 1) % 4]);
        lum[i] = 255.0f - len * 255.0f;
    }

    for (int i = 0; i < 4; ++i) {
        x[i] = x[i] / 8.0f + 0.75f;
        x[i] *= width;
    }

    for (int i = 0; i < 4; ++i) {
        int xi = int(x[i]);
        int xi1 = int(x[(i + 1) % 4]);
        if (xi < xi1)
            texLine(pixels, pitch, xi, xi1, v, int(lum[i]), tex, width);
    }
}

void Twister::draw(uint32_t *pixels, uint16_t height, uint16_t width) {

    if (!texLoaded) {
        // Load the texture from a file
        tex = DecodePng("resources/Honey2_Light.png");
        if (tex.data.empty()) {
            std::cerr << "Failed to load texture for Twister background." << std::endl;
            return;
        }
        // Load the texture from a file
        tex2 = DecodePng("resources/Honey2_Dark.png");
        if (tex.data.empty()) {
            std::cerr << "Failed to load texture for Twister background." << std::endl;
            return;
        }

        texLoaded = true; // Set the flag to true after loading the texture
    }


    // Now we have image (RGBA 8bit data), img_width, img_height
    // Fill the destination buffer
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {

            // Repeat or clip the image as required
            unsigned src_x = (x < tex2.w) ? x : (x % tex2.w);
            unsigned src_y = (y < tex2.h) ? y : (y % tex2.h);

            // Index into the source image (RGBA, so 4 bytes per pixel)
            size_t src_index = (src_y * tex.w + src_x) * 4;

            uint8_t r = tex2.data[src_index + 0] >> 2;
            uint8_t g = tex2.data[src_index + 1] >> 2;
            uint8_t b = tex2.data[src_index + 2] >> 2;
            uint8_t a = tex2.data[src_index + 3];

            // You can choose any pixel format. Let's use ARGB here.
            pixels[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    float vamp = 0.0f;
    float roto = 0.0f;

    for (int v = 0; v < height; ++v) {
        float fv = float(v) / height;

        // Periodic variation over height (v), like a sine wave from top to bottom
        float phaseOffset = std::sin(fv * 10.0f + theta) * 0.5f;   // 10.0 = freq, 1.0 = phase depth
        float amplitude    = 0.90f + 0.10f * std::sin(fv * 6.0f + theta * 1.5f); // 6.0 = vertical cycles, 0.10 = how much amplitude changes

        float x[4];
        for (int i = 0; i < 4; ++i) {
            float localTheta = theta + float(i) * (PI / 2) + phaseOffset;
            x[i] = amplitude * std::sin(vamp * fv + roto + localTheta);
        }

        rasterScan(pixels, 0, v, x, tex, width);
        
    }

    theta += 0.035f;

    setNeedsUpdate(true);

}