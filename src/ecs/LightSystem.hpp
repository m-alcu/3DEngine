#pragma once
#include "Registry.hpp"
#include "../constants.hpp"
#include "../events/Event.hpp"

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

    // Ensure every light has a shadow map allocated
    inline void ensureShadowMaps(ComponentStore<LightComponent>& lights,
                                  sage::Event& pcfRadiusChanged, int pcfRadius) {
        for (auto& [entity, light] : lights) {
            if (!light.shadowMap) {
                light.shadowMap = std::make_shared<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
                light.shadowMap->subscribeToPcfRadiusChanges(pcfRadiusChanged, pcfRadius);
            }
        }
    }

} // namespace LightSystem
