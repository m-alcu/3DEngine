#pragma once

#include <cstdint>
#include <cmath>
#include "background.hpp"
#include "../slib.hpp"
#include "../color.hpp"
#include "../constants.hpp"
#include "../vendor/lodepng/lodepng.h"

class Twister : public Background {

    public:
        void draw(uint32_t *pixels, uint16_t height, uint16_t width);

    private:

        slib::texture tex;
        slib::texture tex2;
        bool texLoaded = false; // Flag to check if texture is loaded
        float theta = 0.0f; // Angle for the twister effect

        void texLine(uint32_t* pixels, int pitch, int x1, int x2, int v, int l, const slib::texture& tex, int width);
        void rasterScan(uint32_t* pixels, int pitch, int v, float* x, const slib::texture& tex, int width);
        slib::texture DecodePng(const char* filename);

};
