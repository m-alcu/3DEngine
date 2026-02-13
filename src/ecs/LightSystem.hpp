#pragma once
#include "Registry.hpp"

namespace LightSystem {

    // Sync light.position from the entity's TransformComponent
    inline void syncPositions(Registry& registry) {
        for (auto& [entity, light] : registry.lights()) {
            TransformComponent* t = registry.transforms().get(entity);
            if (t) {
                light.light.position = {t->position.x, t->position.y, t->position.z};
            }
        }
    }

} // namespace LightSystem
