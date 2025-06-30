#pragma once

#include <cstdint>
#include "background.hpp"
#include "../vendor/lodepng/lodepng.h"

class Imagepng : public Background {

    public:
        void draw(uint32_t *pixels, uint16_t height, uint16_t width);

};
