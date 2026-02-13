#pragma once
#include "TransformComponent.hpp"
#include "ComponentStore.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include <algorithm>
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

    inline slib::vec3 getWorldCenter(const TransformComponent& t,
                                      const slib::vec3& minCoord,
                                      const slib::vec3& maxCoord) {
        slib::vec3 localCenter{(minCoord.x + maxCoord.x) * 0.5f,
                               (minCoord.y + maxCoord.y) * 0.5f,
                               (minCoord.z + maxCoord.z) * 0.5f};
        slib::vec4 world = t.modelMatrix * slib::vec4(localCenter, 1.0f);
        return {world.x, world.y, world.z};
    }

    inline void updateWorldBounds(const TransformComponent& t,
                                   const slib::vec3& minCoord,
                                   const slib::vec3& maxCoord,
                                   slib::vec3& worldBoundMin,
                                   slib::vec3& worldBoundMax) {
        slib::vec3 corners[8] = {
            {minCoord.x, minCoord.y, minCoord.z}, {minCoord.x, minCoord.y, maxCoord.z},
            {minCoord.x, maxCoord.y, minCoord.z}, {minCoord.x, maxCoord.y, maxCoord.z},
            {maxCoord.x, minCoord.y, minCoord.z}, {maxCoord.x, minCoord.y, maxCoord.z},
            {maxCoord.x, maxCoord.y, minCoord.z}, {maxCoord.x, maxCoord.y, maxCoord.z}};

        for (const auto& corner : corners) {
            slib::vec4 world = t.modelMatrix * slib::vec4(corner, 1.0f);
            worldBoundMin.x = std::min(worldBoundMin.x, world.x);
            worldBoundMin.y = std::min(worldBoundMin.y, world.y);
            worldBoundMin.z = std::min(worldBoundMin.z, world.z);
            worldBoundMax.x = std::max(worldBoundMax.x, world.x);
            worldBoundMax.y = std::max(worldBoundMax.y, world.y);
            worldBoundMax.z = std::max(worldBoundMax.z, world.z);
        }
    }

    inline void scaleToRadius(TransformComponent& t, float boundingRadius, float targetRadius) {
        if (boundingRadius > 0.0f) {
            t.position.zoom *= targetRadius / boundingRadius;
        }
    }

    // --- Orbit functions ---

    namespace detail {
        inline float wrapTwoPi(float a) {
            const float TWO_PI = 6.2831853071795864769f;
            a = std::fmod(a, TWO_PI);
            if (a < 0) a += TWO_PI;
            return a;
        }
    }

    inline void buildOrbitBasis(TransformComponent& t, const slib::vec3& n) {
        slib::vec3 a = (std::fabs(n.x) < 0.9f) ? slib::vec3{1,0,0} : slib::vec3{0,1,0};
        t.orbitU = smath::normalize(smath::cross(n, a));
        t.orbitV = smath::normalize(smath::cross(n, t.orbitU));
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

        slib::vec3 P = t.orbit.center + (t.orbitU * c + t.orbitV * s) * t.orbit.radius;

        t.position.x = P.x;
        t.position.y = P.y;
        t.position.z = P.z;
    }

    // --- Batch system functions ---

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
