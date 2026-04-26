#pragma once
#include "../slib.hpp"
#include "../smath.hpp"

struct Position {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float zoom = 1.0f;
    float xAngle = 0.0f;
    float yAngle = 0.0f;
    float zAngle = 0.0f;
};

struct OrbitState {
    slib::vec3 center{ 0,0,0 };
    float radius = 1.0f;
    slib::vec3 n{ 0,1,0 };
    float omega = 1.0f;
    float phase = 0.0f;
    bool enabled = false;
    slib::vec3 u{ 1,0,0 };
    slib::vec3 v{ 0,0,1 };
};

struct TransformComponent {
    Position position;
    slib::mat4 modelMatrix{smath::identity()};
    slib::mat4 normalMatrix{smath::identity()};

    bool autoRotate = false;
    float incXangle = 0.0f;
    float incYangle = 0.0f;

    OrbitState orbit;
};
