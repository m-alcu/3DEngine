#pragma once
#include "slib.hpp"

inline static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

class Camera {
public:
    slib::vec3 pos{ 0, 0, 0 };
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
    slib::vec3 forward{ 0, 0, 0 };
    float eagerness = 0.1f;
    float sensitivity = 0.05f;
    float speed = 25.0f;

    // Orbit parameters
    slib::vec3 orbitTarget{ 0, 0, 0 };
    float orbitRadius = 5.0f;
    float orbitAzimuth = 0.0f;
    float orbitElevation = 0.0f;

    // Projection parameters
    float zNear = 10.0f;      // Near plane distance
    float zFar = 10000.0f;    // Far plane distance
    float viewAngle = 45.0f;  // Field of view angle in degrees

    Camera() = default;

    void setOrbitFromCurrent() {
        slib::vec3 d = pos - orbitTarget;
        orbitRadius = smath::distance(d);
        orbitAzimuth = std::atan2(d.x, d.z);
        orbitElevation = std::asin(d.y / orbitRadius);
    }

    void applyOrbit() {
        const float el = clampf(orbitElevation, -1.5533f, 1.5533f); // ~�89�
        const float ca = std::cos(orbitAzimuth), sa = std::sin(orbitAzimuth);
        const float ce = std::cos(el), se = std::sin(el);

        slib::vec3 offset{
            orbitRadius * sa * ce,
            orbitRadius * se,
            orbitRadius * ca * ce
        };

        pos = orbitTarget + offset;
        forward = smath::normalize(orbitTarget - pos);
        yaw = std::atan2(forward.x, -forward.z);
        pitch = std::asin(-forward.y);
    }
};