#pragma once
#include <vector>
#include <cstdint>
#include <cmath>

enum class TextureFilter {
    NEIGHBOUR,
    BILINEAR
};

class Texture {
public:
    int w = 0;
    int h = 0;
    std::vector<unsigned char> data;
    unsigned int bpp = 4;
    unsigned int rowStride = 0; // == w * bpp (set once)
    TextureFilter textureFilter = TextureFilter::NEIGHBOUR;

    // Sample texture at normalized coordinates (u, v) in [0, 1]
    // Returns RGB values in [0, 255]
    void sampleNearest(float u, float v, int& r, int& g, int& b) const {
        int tx = static_cast<int>(u * w);
        if (tx == w) tx = w - 1;

        int ty = static_cast<int>(v * h);
        if (ty == h) ty = h - 1;

        const uint32_t* row32 = reinterpret_cast<const uint32_t*>(
            &data[static_cast<size_t>(ty) * static_cast<size_t>(rowStride)]);
        uint32_t px = row32[tx];

        r = (px) & 0xFF;
        g = (px >> 8) & 0xFF;
        b = (px >> 16) & 0xFF;
    }

    // Sample texture with bilinear filtering at normalized coordinates (u, v) in [0, 1]
    // Returns RGB values in [0, 255] as floats for interpolation precision
    void sampleBilinear(float u, float v, float& r, float& g, float& b) const {
        // Map to texel space and center on texel centers (-0.5)
        float xf = u * w - 0.5f;
        float yf = v * h - 0.5f;

        // Floor once; clamp to keep (x+1,y+1) in-bounds
        int x = static_cast<int>(std::floor(xf));
        int y = static_cast<int>(std::floor(yf));
        if (x < 0) x = 0; else if (x > w - 2) x = w - 2;
        if (y < 0) y = 0; else if (y > h - 2) y = h - 2;

        float fx = xf - x;
        float fy = yf - y;

        // Precompute row bases (in bytes)
        const uint8_t* rowT = data.data() + y * rowStride;
        const uint8_t* rowB = rowT + rowStride;

        // Assume RGBA8 (bpp = 4)
        const uint32_t* rowT32 = reinterpret_cast<const uint32_t*>(rowT);
        const uint32_t* rowB32 = reinterpret_cast<const uint32_t*>(rowB);

        // Load 4 neighboring pixels
        uint32_t p00 = rowT32[x];
        uint32_t p10 = rowT32[x + 1];
        uint32_t p01 = rowB32[x];
        uint32_t p11 = rowB32[x + 1];

        // Horizontal lerp on top and bottom rows, then vertical lerp
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

        float rT = lerp(static_cast<float>(p00 & 0xFF),
                        static_cast<float>(p10 & 0xFF), fx);
        float gT = lerp(static_cast<float>((p00 >> 8) & 0xFF),
                        static_cast<float>((p10 >> 8) & 0xFF), fx);
        float bT = lerp(static_cast<float>((p00 >> 16) & 0xFF),
                        static_cast<float>((p10 >> 16) & 0xFF), fx);

        float rB = lerp(static_cast<float>(p01 & 0xFF),
                        static_cast<float>(p11 & 0xFF), fx);
        float gB = lerp(static_cast<float>((p01 >> 8) & 0xFF),
                        static_cast<float>((p11 >> 8) & 0xFF), fx);
        float bB = lerp(static_cast<float>((p01 >> 16) & 0xFF),
                        static_cast<float>((p11 >> 16) & 0xFF), fx);

        r = lerp(rT, rB, fy);
        g = lerp(gT, gB, fy);
        b = lerp(bT, bB, fy);
    }

    // Unified sample method using the texture's filter setting
    // For use with perspective-correct texture coordinates (u/w, v/w, 1/w)
    void sample(float u, float v, float& r, float& g, float& b) const {
        if (textureFilter == TextureFilter::BILINEAR) {
            sampleBilinear(u, v, r, g, b);
        } else {
            int ri, gi, bi;
            sampleNearest(u, v, ri, gi, bi);
            r = static_cast<float>(ri);
            g = static_cast<float>(gi);
            b = static_cast<float>(bi);
        }
    }
};

