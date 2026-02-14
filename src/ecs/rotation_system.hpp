#pragma once
#include "registry.hpp"
#include "transform_system.hpp"

namespace RotationSystem {

    // Apply rotation increments to all entities with a RotationComponent
    inline void updateAll(Registry& registry) {
        for (auto& [entity, rot] : registry.rotations()) {
            if (rot.enabled) {
                TransformComponent* t = registry.transforms().get(entity);
                if (t) {
                    TransformSystem::incAngles(*t, rot.incXangle, rot.incYangle, 0.0f);
                }
            }
        }
    }

} // namespace RotationSystem
