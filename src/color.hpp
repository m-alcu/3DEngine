#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "slib.hpp"

class Color : public slib::vec3 {
public:
    // Default constructor: black (0, 0, 0)
    constexpr Color() : slib::vec3(0.0f, 0.0f, 0.0f) {}

    // Constructor with red, green, blue components
    constexpr Color(float r, float g, float b) : slib::vec3(r, g, b) {}

    // Constructor from slib::vec3 directly
    constexpr Color(const slib::vec3& v) : slib::vec3(v) {}

    uint32_t toBgra() const {
        auto toU8 = [](float val) -> uint32_t {
            if (val <= 0.0f) return 0u;
            if (val >= 255.0f) return 255u;
            return static_cast<uint32_t>(val + 0.5f);
        };

        return 0xff000000u |
               (toU8(x) << 16) |
               (toU8(y) <<  8) |
                toU8(z);
    }

    float& red()   { return x; }
    float& green() { return y; }
    float& blue()  { return z; }

    const float& red()   const { return x; }
    const float& green() const { return y; }
    const float& blue()  const { return z; }
};
