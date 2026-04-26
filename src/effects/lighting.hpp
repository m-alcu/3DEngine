#pragma once
#include <cmath>
#include "../scene.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/entity.hpp"

namespace lighting {

inline float sampleShadow(const Scene& scene, Entity entity,
                           const slib::vec3& worldPos, float diff,
                           const slib::vec3& lightPos) {
    const auto* sc = scene.shadows().get(entity);
    if (!scene.shadowsEnabled || !sc || !sc->shadowMap) return 1.0f;
    return sc->shadowMap->sampleShadow(worldPos, diff, lightPos);
}

// Phong: R = 2(N·L)N - L, dot(R, -camForward)
// Blinn-Phong: H = normalize(L - camForward), dot(N, H)
inline float specular(const slib::vec3& N, const slib::vec3& L,
                      const slib::vec3& camForward, float Ns, bool blinn) {
    if (blinn) {
        slib::vec3 H = smath::normalize(L - camForward);
        return std::pow(std::max(0.0f, smath::dot(N, H)), Ns);
    } else {
        slib::vec3 R = N * 2.0f * smath::dot(N, L) - L;
        return std::pow(std::max(0.0f, smath::dot(R, camForward * -1.0f)), Ns);
    }
}

} // namespace lighting
