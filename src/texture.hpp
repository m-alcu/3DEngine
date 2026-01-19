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

        int x = static_cast<int>(xf);
        int y = static_cast<int>(yf);
        // Clamp to keep (x+1, y+1) in-bounds
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

        // Load 4 neighboring pixels and extract RGB components once
        uint32_t p00 = rowT32[x];
        uint32_t p10 = rowT32[x + 1];
        uint32_t p01 = rowB32[x];
        uint32_t p11 = rowB32[x + 1];

        // Extract color components as floats (single conversion per component)
        float r00 = static_cast<float>(p00 & 0xFF);
        float g00 = static_cast<float>((p00 >> 8) & 0xFF);
        float b00 = static_cast<float>((p00 >> 16) & 0xFF);

        float r10 = static_cast<float>(p10 & 0xFF);
        float g10 = static_cast<float>((p10 >> 8) & 0xFF);
        float b10 = static_cast<float>((p10 >> 16) & 0xFF);

        float r01 = static_cast<float>(p01 & 0xFF);
        float g01 = static_cast<float>((p01 >> 8) & 0xFF);
        float b01 = static_cast<float>((p01 >> 16) & 0xFF);

        float r11 = static_cast<float>(p11 & 0xFF);
        float g11 = static_cast<float>((p11 >> 8) & 0xFF);
        float b11 = static_cast<float>((p11 >> 16) & 0xFF);

        // Precompute interpolation weights
        float fx1 = 1.0f - fx;
        float fy1 = 1.0f - fy;

        // Bilinear interpolation using weighted sum (faster than nested lerps)
        r = (r00 * fx1 + r10 * fx) * fy1 + (r01 * fx1 + r11 * fx) * fy;
        g = (g00 * fx1 + g10 * fx) * fy1 + (g01 * fx1 + g11 * fx) * fy;
        b = (b00 * fx1 + b10 * fx) * fy1 + (b01 * fx1 + b11 * fx) * fy;
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

