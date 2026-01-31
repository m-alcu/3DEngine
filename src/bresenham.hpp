#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include "polygon.hpp"
#include "ZBuffer.hpp"

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

template<typename Vertex>
void drawWireframePolygon(Polygon<Vertex>& polygon, uint32_t color,
                          uint32_t* pixels, int screenWidth, int screenHeight,
                          ZBuffer* zBuffer) {
    const size_t n = polygon.points.size();
    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        auto& v0 = polygon.points[j];
        auto& v1 = polygon.points[i];

        drawBresenhamLine(v0.p_x >> 16, v0.p_y >> 16, v0.p_z,
                          v1.p_x >> 16, v1.p_y >> 16, v1.p_z,
                          pixels, color, screenWidth, screenHeight, zBuffer);
    }
}
