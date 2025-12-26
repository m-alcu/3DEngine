#include <iostream>
#include <math.h>
#include "scene.hpp"
#include "constants.hpp"

void Scene::setup() {
}

inline static float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

void Scene::cameraSetOrbitFromCurrent(Camera& cam) {
    slib::vec3 d = cam.pos - cam.orbitTarget;
    cam.orbitRadius = smath::distance(d);
    // Spherical: azimuth about +Y, elevation from XZ-plane
    cam.orbitAzimuth = std::atan2(d.x, d.z);              // [-pi, pi]
    cam.orbitElevation = std::asin(d.y / cam.orbitRadius);  // [-pi/2, pi/2]
}

void Scene::cameraApplyOrbit(Camera& cam) {
    const float el = clampf(cam.orbitElevation, -1.5533f, 1.5533f); // ~±89°
    const float ca = std::cos(cam.orbitAzimuth), sa = std::sin(cam.orbitAzimuth);
    const float ce = std::cos(el), se = std::sin(el);

    // Spherical to Cartesian, Y-up
    slib::vec3 offset{
        cam.orbitRadius * sa * ce,
        cam.orbitRadius * se,
        cam.orbitRadius * ca * ce
    };

    cam.pos = cam.orbitTarget + offset;

    // Look at target
    cam.forward = smath::normalize(cam.orbitTarget - cam.pos);
    // Optionally refresh Euler yaw/pitch so your existing code stays in sync:
    cam.yaw = std::atan2(cam.forward.x, -cam.forward.z);
    cam.pitch = std::asin(-cam.forward.y);

}

