#pragma once
#include <cmath>
#include "constants.hpp"
#include "slib.hpp"
#include "smath.hpp"

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
    float zNear = CAMERA_DEFAULT_ZNEAR;      // Near plane distance
    float zFar = CAMERA_DEFAULT_ZFAR;    // Far plane distance
    float viewAngle = CAMERA_DEFAULT_VIEW_ANGLE;  // Field of view angle in degrees

    Camera() = default;

    void setOrbitFromCurrent() {
        slib::vec3 d = pos - orbitTarget;
        orbitRadius = smath::distance(d);
        orbitAzimuth = std::atan2(d.x, d.z);
        orbitElevation = std::asin(d.y / orbitRadius);
    }

    void applyOrbit() {
        const float el = clampf(orbitElevation, -1.5533f, 1.5533f); // ~89º
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

    slib::mat4 projectionMatrix(float aspectRatio) const {
        return smath::perspective(zFar, zNear, aspectRatio, viewAngle * RAD);
    }

    slib::mat4 viewMatrix(bool orbiting) const {
        if (orbiting) {
            slib::vec3 up = {0.0f, 1.0f, 0.0f};
            slib::vec3 dir = smath::normalize(orbitTarget - pos);
            // gimbal lock avoidance check https://en.wikipedia.org/wiki/Gimbal_lock
            if (std::abs(smath::dot(dir, up)) > 0.99f)
                up = {1.0f, 0.0f, 0.0f};
            return smath::lookAt(pos, orbitTarget, up);
        }
        return smath::fpsview(pos, pitch, yaw, roll);
    }

    slib::vec3 forwardNeg() const { return {-forward.x, -forward.y, -forward.z}; }

    bool isVisibleFromCamera(const slib::vec3& world, const slib::vec3& faceNormal) const {
        slib::vec3 viewDir = pos - world;
        float dotResult = smath::dot(faceNormal, smath::normalize(viewDir));
        return dotResult > 0.0f;
    }
};
