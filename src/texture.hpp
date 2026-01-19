#pragma once
#include <vector>
#include <cstdint>
#include <cmath>

enum class TextureFilter {
    NEIGHBOUR,
    BILINEAR
};

struct RGBA8 {
    uint8_t r, g, b, a;
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

        const RGBA8* row = reinterpret_cast<const RGBA8*>(
            &data[static_cast<size_t>(ty) * static_cast<size_t>(rowStride)]);
        const RGBA8& px = row[tx];

        r = px.r;
        g = px.g;
        b = px.b;
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

        // Access pixels as RGBA8 structs (avoids bit shifting)
        const RGBA8* rowT8 = reinterpret_cast<const RGBA8*>(rowT);
        const RGBA8* rowB8 = reinterpret_cast<const RGBA8*>(rowB);

        // Load 4 neighboring pixels
        const RGBA8& p00 = rowT8[x];
        const RGBA8& p10 = rowT8[x + 1];
        const RGBA8& p01 = rowB8[x];
        const RGBA8& p11 = rowB8[x + 1];

        // Direct byte access - no bit shifting needed
        float r00 = static_cast<float>(p00.r);
        float g00 = static_cast<float>(p00.g);
        float b00 = static_cast<float>(p00.b);

        float r10 = static_cast<float>(p10.r);
        float g10 = static_cast<float>(p10.g);
        float b10 = static_cast<float>(p10.b);

        float r01 = static_cast<float>(p01.r);
        float g01 = static_cast<float>(p01.g);
        float b01 = static_cast<float>(p01.b);

        float r11 = static_cast<float>(p11.r);
        float g11 = static_cast<float>(p11.g);
        float b11 = static_cast<float>(p11.b);

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

