#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "slib.hpp"

class Color : public slib::vec3 {
public:
    // Default constructor: black (0, 0, 0)
    constexpr Color() : slib::vec3(0.0f, 0.0f, 0.0f) {}

    // Constructor with blue, green, red components
    constexpr Color(float b, float g, float r) : slib::vec3(b, g, r) {}

    // Constructor from slib::vec3 directly
    constexpr Color(const slib::vec3& v) : slib::vec3(v) {}

    uint32_t toBgra() const {
        // Clamp and convert to int in one step
        auto clamp = [](float val) -> int {
            if (val <= 0.0f) return 0;
            if (val >= 255.0f) return 255;
            return static_cast<int>(val);
        };

        return 0xff000000u |
               (static_cast<uint32_t>(clamp(x)) << 16) |
               (static_cast<uint32_t>(clamp(y)) << 8) |
               static_cast<uint32_t>(clamp(z));
    }

    float& blue()  { return x; }
    float& green() { return y; }
    float& red()   { return z; }

    const float& blue()  const { return x; }
    const float& green() const { return y; }
    const float& red()   const { return z; }
};
