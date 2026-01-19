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
    TextureFilter textureFilter = TextureFilter::NEIGHBOUR;

    // Get pixels as RGBA8 array
    const RGBA8* pixels() const {
        return reinterpret_cast<const RGBA8*>(data.data());
    }

    // Sample texture at normalized coordinates (u, v) in [0, 1]
    // Returns RGB values in [0, 255] as floats
    void sampleNearest(float u, float v, float& r, float& g, float& b) const {
        int tx = static_cast<int>(u * w);
        if (tx == w) tx = w - 1;

        int ty = static_cast<int>(v * h);
        if (ty == h) ty = h - 1;

        const RGBA8& px = pixels()[ty * w + tx];

        r = static_cast<float>(px.r);
        g = static_cast<float>(px.g);
        b = static_cast<float>(px.b);
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

        // Direct 2D indexing with RGBA8 pointer
        const RGBA8* px = pixels();
        const RGBA8* rowT8 = px + y * w;
        const RGBA8* rowB8 = px + (y + 1) * w;

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
            sampleNearest(u, v, r, g, b);
        }
    }
};

