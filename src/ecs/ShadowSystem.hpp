#pragma once
#include "ShadowComponent.hpp"
#include "ComponentStore.hpp"
#include "../constants.hpp"
#include "../slib.hpp"

namespace ShadowSystem {

    inline void ensureShadowMaps(ComponentStore<ShadowComponent>& shadows,
                                 int pcfRadius) {
        for (auto& [entity, shadow] : shadows) {
            if (!shadow.shadowMap) {
                shadow.shadowMap = std::make_shared<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
            }
            shadow.shadowMap->pcfRadius = pcfRadius;
        }
    }

    inline void clearShadowMaps(ComponentStore<ShadowComponent>& shadows) {
        for (auto& [entity, shadow] : shadows) {
            if (shadow.shadowMap) {
                shadow.shadowMap->clear();
            }
        }
    }

    inline void buildLightMatrices(ShadowComponent& shadow,
                                   const Light& light,
                                   const slib::vec3& sceneCenter,
                                   float sceneRadius) {
        if (shadow.shadowMap) {
            shadow.shadowMap->buildLightMatrices(light, sceneCenter, sceneRadius);
        }
    }

} // namespace ShadowSystem
