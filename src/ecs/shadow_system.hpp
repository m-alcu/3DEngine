#pragma once
#include "shadow_component.hpp"
#include "component_store.hpp"
#include "../constants.hpp"
#include "../slib.hpp"
#include <cstdio>

namespace ShadowSystem {

    inline void ensureShadowMaps(ComponentStore<ShadowComponent>& shadows,
                                 ComponentStore<LightComponent>& lights,
                                 int pcfRadius,
                                 bool useCubemapForPointLights = false) {
        for (auto& [entity, shadow] : shadows) {
            // Check if this is a point light
            bool isPointLight = false;
            auto* lightComp = lights.get(entity);
            if (lightComp && lightComp->light.type == LightType::Point) {
                isPointLight = true;
            }
            
            // Determine if cubemap should be enabled for this light
            bool enableCubemap = isPointLight && useCubemapForPointLights;
            
            // Create or recreate shadow map if needed
            if (!shadow.shadowMap || shadow.shadowMap->useCubemap != enableCubemap) {
                shadow.shadowMap = std::make_shared<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, enableCubemap);
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
                                   float sceneRadius,
                                   float minBiasDefault = MIN_BIAS_DEFAULT,
                                   float maxBiasDefault = MAX_BIAS_DEFAULT,
                                   float shadowBiasMin = SHADOW_BIAS_MIN,
                                   float shadowBiasMax = SHADOW_BIAS_MAX) {
        if (shadow.shadowMap) {
            shadow.shadowMap->buildLightMatrices(light, sceneCenter, sceneRadius,
                                                minBiasDefault, maxBiasDefault,
                                                shadowBiasMin, shadowBiasMax);
        }
    }

} // namespace ShadowSystem
