#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include "z_buffer.hpp"

inline void drawBresenhamLine(int x0, int y0, float z0, int x1, int y1, float z1,
                              uint32_t* pixels, uint32_t color,
                              int screenWidth, int screenHeight, ZBuffer* zBuffer) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int steps = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    float z = z0;
    float zStep = steps > 0 ? (z1 - z0) / steps : 0.0f;
    int err = dx + dy, e2;

    while (true) {
        if (x0 >= 0 && x0 < screenWidth && y0 >= 0 && y0 < screenHeight) {
            int pos = y0 * screenWidth + x0;
            if (zBuffer->TestAndSet(pos, z)) {
                pixels[pos] = color;
            }
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
        z += zStep;
    }
}

inline void drawBresenhamLine(int x0, int y0, int x1, int y1,
                              uint32_t* pixels, uint32_t color,
                              int screenWidth, int screenHeight) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        if (x0 >= 0 && x0 < screenWidth && y0 >= 0 && y0 < screenHeight)
            pixels[y0 * screenWidth + x0] = color;

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

