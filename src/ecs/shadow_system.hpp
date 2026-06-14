#pragma once
#include "shadow_component.hpp"
#include "component_store.hpp"
#include "../constants.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include "../light.hpp"
#include <algorithm>
#include <cmath>

// ECS system owning shadow-map logic: allocation/configuration and the
// construction of light-space matrices from a light + scene bounds.
// ShadowMap itself only stores depth and samples it.
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

    namespace detail {

        inline void buildDirectional(ShadowMap& map, const Light& light,
                                     const slib::vec3& sceneCenter, float sceneRadius) {
            slib::vec3 lightDir = smath::normalize(light.direction);
            slib::vec3 lightPos = sceneCenter + lightDir * sceneRadius * 2.0f;

            slib::vec3 up = {0.0f, 1.0f, 0.0f};
            if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
                up = {1.0f, 0.0f, 0.0f};
            }

            slib::mat4 lightView = smath::lookAt(lightPos, sceneCenter, up);

            float size = sceneRadius * 1.2f;
            map.lightProjMatrix =
                smath::ortho(-size, size, -size, size, 0.1f, sceneRadius * 4.0f);
            map.lightSpaceMatrices[0] = lightView * map.lightProjMatrix;
        }

        inline void buildPoint(ShadowMap& map, const Light& light,
                              const slib::vec3& sceneCenter, float sceneRadius) {
            slib::vec3 up = {0.0f, 1.0f, 0.0f};
            slib::vec3 lightDir = smath::normalize(sceneCenter - light.position);
            if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
                up = {1.0f, 0.0f, 0.0f};
            }

            slib::mat4 lightView = smath::lookAt(light.position, sceneCenter, up);

            slib::vec3 toScene = sceneCenter - light.position;
            float distToScene = std::sqrt(smath::dot(toScene, toScene));
            distToScene = std::max(distToScene, 1.0f);
            float effectiveRadius = sceneRadius * EFFECTIVE_LIGHT_RADIUS_FACTOR;
            float fov = 2.0f * std::atan(effectiveRadius / distToScene);
            fov = std::clamp(fov, 20.0f * RAD, 90.0f * RAD);

            float aspect = static_cast<float>(map.faceWidth) / map.faceHeight;

            float zNear, zFar;
            if (distToScene > sceneRadius * 1.5f) {
                zNear = std::max(1.0f, distToScene - sceneRadius);
                zFar = distToScene + sceneRadius * 2.0f;
            } else {
                zNear = std::max(0.1f, distToScene * 0.05f);
                zFar = std::max(zNear * 2.0f, distToScene + sceneRadius * 1.2f);
            }

            const float maxDepthRatio = 300.0f;
            if (zFar / zNear > maxDepthRatio) {
                zNear = zFar / maxDepthRatio;
            }

            map.lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);
            map.lightSpaceMatrices[0] = lightView * map.lightProjMatrix;
        }

        inline void buildCubemap(ShadowMap& map, const Light& light, float sceneRadius) {
            map.zNear = std::max(10.0f, sceneRadius * 0.01f);
            map.zFar = std::max(light.radius * 2.0f, sceneRadius * 3.0f);

            float aspect = 1.0f;
            float fov = PI / 2.0f;
            map.lightProjMatrix = smath::perspective(map.zFar, map.zNear, aspect, fov);

            const slib::vec3& pos = light.position;

            // +X, -X, +Y, -Y, +Z, -Z
            slib::mat4 views[6] = {
                smath::lookAt(pos, pos + slib::vec3{1, 0, 0}, {0, -1, 0}),
                smath::lookAt(pos, pos + slib::vec3{-1, 0, 0}, {0, -1, 0}),
                smath::lookAt(pos, pos + slib::vec3{0, 1, 0}, {0, 0, 1}),
                smath::lookAt(pos, pos + slib::vec3{0, -1, 0}, {0, 0, -1}),
                smath::lookAt(pos, pos + slib::vec3{0, 0, 1}, {0, -1, 0}),
                smath::lookAt(pos, pos + slib::vec3{0, 0, -1}, {0, -1, 0}),
            };

            for (int i = 0; i < 6; ++i) {
                map.lightSpaceMatrices[i] = views[i] * map.lightProjMatrix;
            }
        }

        inline void buildSpot(ShadowMap& map, const Light& light, float sceneRadius) {
            slib::vec3 lightDir = smath::normalize(light.direction);
            slib::vec3 target = light.position + lightDir * light.radius;

            slib::vec3 up = {0.0f, 1.0f, 0.0f};
            if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
                up = {1.0f, 0.0f, 0.0f};
            }

            slib::mat4 lightView = smath::lookAt(light.position, target, up);

            float fov = std::acos(light.outerCutoff) * 2.0f;
            float aspect = static_cast<float>(map.faceWidth) / map.faceHeight;
            float zNear = 1.0f;
            float zFar = light.radius * 2.0f;

            map.lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);
            map.lightSpaceMatrices[0] = lightView * map.lightProjMatrix;
        }

    } // namespace detail

    inline void buildLightMatrices(ShadowMap& map, const Light& light,
                                   const slib::vec3& sceneCenter, float sceneRadius) {
        switch (light.type) {
        case LightType::Directional:
            detail::buildDirectional(map, light, sceneCenter, sceneRadius);
            break;
        case LightType::Point:
            if (map.numFaces == 6) {
                detail::buildCubemap(map, light, sceneRadius);
            } else {
                detail::buildPoint(map, light, sceneCenter, sceneRadius);
            }
            break;
        case LightType::Spot:
            detail::buildSpot(map, light, sceneRadius);
            break;
        }
    }

} // namespace ShadowSystem
