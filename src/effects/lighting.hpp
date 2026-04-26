#pragma once
#include "../scene.hpp"
#include "../slib.hpp"
#include "../ecs/entity.hpp"

namespace lighting {

inline float sampleShadow(const Scene& scene, Entity entity,
                           const slib::vec3& worldPos, float diff,
                           const slib::vec3& lightPos) {
    const auto* sc = scene.shadows().get(entity);
    if (!scene.shadowsEnabled || !sc || !sc->shadowMap) return 1.0f;
    return sc->shadowMap->sampleShadow(worldPos, diff, lightPos);
}

} // namespace lighting
