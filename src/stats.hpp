#pragma once
#include <cstdint>

class Stats {
public:
    uint32_t polysRendered = 0;
    uint32_t pixelsRasterized = 0;

    void reset() {
        polysRendered = 0;
        pixelsRasterized = 0;
    }

    void addPoly() {
        ++polysRendered;
    }

    void addPixel() {
        ++pixelsRasterized;
    }
};
