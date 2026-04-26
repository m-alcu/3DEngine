#pragma once
#include "transform_component.hpp"
#include "component_store.hpp"
#include "../constants.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include <cmath>

namespace TransformSystem {

    inline void updateTransform(TransformComponent& t) {
        slib::mat4 rotate = smath::rotation(
            slib::vec3{t.position.xAngle, t.position.yAngle, t.position.zAngle});
        slib::mat4 translate = smath::translation(
            slib::vec3{t.position.x, t.position.y, t.position.z});
        slib::mat4 scale = smath::scale(
            slib::vec3{t.position.zoom, t.position.zoom, t.position.zoom});
        t.modelMatrix = translate * rotate * scale;
        t.normalMatrix = rotate;
    }

    inline slib::vec3 rotateNormal(const TransformComponent& t, const slib::vec3& normal) {
        slib::vec4 rotated = t.normalMatrix * slib::vec4(normal, 0);
        return {rotated.x, rotated.y, rotated.z};
    }

    inline void incAngles(TransformComponent& t, float xAngle, float yAngle, float zAngle) {
        t.position.xAngle += xAngle;
        t.position.yAngle += yAngle;
        t.position.zAngle += zAngle;
    }

    inline slib::vec3 getWorldCenter(const TransformComponent& t) {
        slib::vec4 world = t.modelMatrix * slib::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        return {world.x, world.y, world.z};
    }

    inline void scaleToRadius(TransformComponent& t, float boundingRadius, float targetRadius) {
        if (boundingRadius > 0.0f) {
            t.position.zoom *= targetRadius / boundingRadius;
        }
    }

    // --- Orbit functions ---

    namespace detail {
        inline float wrapTwoPi(float a) {
            a = std::fmod(a, 2.0f * PI);
            if (a < 0) a += 2.0f * PI;
            return a;
        }
    }

    inline void buildOrbitBasis(TransformComponent& t, const slib::vec3& n) {
        slib::vec3 a = (std::fabs(n.x) < 0.9f) ? slib::vec3{1,0,0} : slib::vec3{0,1,0};
        t.orbit.u = smath::normalize(smath::cross(n, a));
        t.orbit.v = smath::normalize(smath::cross(n, t.orbit.u));
    }

    inline void enableCircularOrbit(TransformComponent& t,
                                     const slib::vec3& center,
                                     float radius,
                                     const slib::vec3& planeNormal,
                                     float angularSpeedRadiansPerSec,
                                     float initialPhaseRadians = 0.0f) {
        t.orbit.center = center;
        t.orbit.radius = radius;
        t.orbit.n = smath::normalize(planeNormal);
        t.orbit.omega = angularSpeedRadiansPerSec;
        t.orbit.phase = initialPhaseRadians;
        t.orbit.enabled = true;
        buildOrbitBasis(t, t.orbit.n);
    }

    inline void disableCircularOrbit(TransformComponent& t) {
        t.orbit.enabled = false;
    }

    inline void updateOrbit(TransformComponent& t, float dt) {
        if (!t.orbit.enabled) return;

        t.orbit.phase = detail::wrapTwoPi(t.orbit.phase + t.orbit.omega * dt);

        float c = std::cos(t.orbit.phase);
        float s = std::sin(t.orbit.phase);

        slib::vec3 P = t.orbit.center + (t.orbit.u * c + t.orbit.v * s) * t.orbit.radius;

        t.position.x = P.x;
        t.position.y = P.y;
        t.position.z = P.z;
    }

    // --- Batch system functions ---

    // Apply per-entity auto-rotation increments
    inline void updateAllRotations(ComponentStore<TransformComponent>& store) {
        for (auto& [entity, t] : store) {
            if (t.autoRotate) {
                incAngles(t, t.incXangle, t.incYangle, 0.0f);
            }
        }
    }

    // Iterate all transforms, update orbit positions
    inline void updateAllOrbits(ComponentStore<TransformComponent>& store, float dt) {
        for (auto& [entity, t] : store) {
            updateOrbit(t, dt);
        }
    }

    // Iterate all transforms, rebuild modelMatrix + normalMatrix
    inline void updateAllTransforms(ComponentStore<TransformComponent>& store) {
        for (auto& [entity, t] : store) {
            updateTransform(t);
        }
    }

} // namespace TransformSystem
