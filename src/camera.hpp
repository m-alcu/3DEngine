#pragma once
#include "slib.hpp"

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

    Camera() = default;
};