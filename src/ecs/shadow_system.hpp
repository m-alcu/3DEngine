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
                                 bool useCubemapForPointLights = false,
                                 float maxSlopeBias = CUBE_SHADOW_MAX_SLOPE_BIAS) {
        for (auto& [entity, shadow] : shadows) {
            bool isPointLight = false;
            auto* lightComp = lights.get(entity);
            if (lightComp && lightComp->light.type == LightType::Point) {
                isPointLight = true;
            }

            int numFaces = (isPointLight && useCubemapForPointLights) ? 6 : 1;

            if (!shadow.shadowMap || shadow.shadowMap->numFaces != numFaces) {
                shadow.shadowMap = std::make_shared<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, numFaces);
            }
            shadow.shadowMap->pcfRadius = pcfRadius;
            shadow.shadowMap->maxSlopeBias = maxSlopeBias;
        }
    }

    inline void clearShadowMaps(ComponentStore<ShadowComponent>& shadows) {
        for (auto& [entity, shadow] : shadows) {
            if (shadow.shadowMap) {
                shadow.shadowMap->setAllDirty();
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
