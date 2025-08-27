#pragma once
#include <cstdint>
#include <cmath>
#include "slib.hpp"   // assuming vec3, dot, normalize, etc.

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light {
public:
    LightType type;        // Light kind
    slib::vec3 color;      // RGB intensity
    float intensity;       // Global strength multiplier

    // Directional
    slib::vec3 direction;  // normalized vector (towards surface)

    // Point / Spot
    slib::vec3 position;   // world-space location
    float radius;          // attenuation radius

    // Spot
    float innerCutoff;     // cos(angle in radians)
    float outerCutoff;     // cos(angle in radians, for smooth edge)

    Light(LightType type = LightType::Directional,
        const slib::vec3& color = { 1.0f, 1.0f, 1.0f },
        float intensity = 1.0f)
        : type(type),
        color(color),
        intensity(intensity),
        direction({ 0.0f, -1.0f, 0.0f }),
        position({ 0.0f, 0.0f, 0.0f }),
        radius(100.0f),
        innerCutoff(std::cos(0.5f)),   // ~60° cone
        outerCutoff(std::cos(0.7f))    // ~80° cone
    {
    }

    // Direction vector from a surface point towards light
    slib::vec3 getDirection(const slib::vec3& surfacePos) const {
        switch (type) {
        case LightType::Directional:
            return smath::normalize( { -direction.x, -direction.y, -direction.z } );
        case LightType::Point:
            return smath::normalize(position - surfacePos);
        case LightType::Spot:
            return smath::normalize(position - surfacePos);
        }
        return { 0,0,0 }; // fallback
    }

    // Attenuation factor (distance falloff + spotlight cone)
    float getAttenuation(const slib::vec3& surfacePos) const {
        if (type == LightType::Directional) {
            return intensity; // no attenuation
        }

        float dist2 = smath::dot(position - surfacePos,
            position - surfacePos);
        float attenuation = intensity / (1.0f + dist2 / (radius * radius));

        if (type == LightType::Spot) {
            slib::vec3 L = smath::normalize(surfacePos - position);
            float theta = smath::dot(L, smath::normalize(direction));

            // smooth falloff between inner/outer cutoff
            float epsilon = innerCutoff - outerCutoff;
            float spotFactor = std::clamp((theta - outerCutoff) / epsilon, 0.0f, 1.0f);

            attenuation *= spotFactor;
        }

        return attenuation;
    }
};
