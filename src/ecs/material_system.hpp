#pragma once
#include <cmath>
#include <string>
#include "material_component.hpp"
#include "../slib.hpp"

enum class MaterialType {
    Rubber,
    Plastic,
    Wood,
    Marble,
    Glass,
    Metal,
    Mirror,
    Light
};

struct MaterialProperties {
    float k_s;
    float k_a;
    float k_d;
    float shininess;
};

namespace MaterialSystem {

    inline MaterialProperties getMaterialProperties(MaterialType type) {
        switch (type) {
            case MaterialType::Rubber:   return {0.1f, 0.2f, 0.5f, 2};
            case MaterialType::Plastic:  return {0.3f, 0.2f, 0.6f, 2};
            case MaterialType::Wood:     return {0.2f, 0.3f, 0.7f, 2};
            case MaterialType::Marble:   return {0.4f, 0.4f, 0.8f, 2};
            case MaterialType::Glass:    return {0.6f, 0.1f, 0.2f, 2};
            case MaterialType::Metal:    return {0.4f, 0.2f, 0.4f, 30};
            case MaterialType::Mirror:   return {1.0f, 0.0f, 0.0f, 2};
            case MaterialType::Light:    return {0.0f, 1.0f, 0.0f, 1};
            default:                     return {0.0f, 0.0f, 0.0f, 0};
        }
    }

    inline Material initDefaultMaterial(const MaterialProperties& properties,
                                        const slib::vec3& kaScale,
                                        const slib::vec3& kdScale,
                                        const slib::vec3& ksScale,
                                        const std::string& texturePath = {},
                                        TextureFilter filter = TextureFilter::NEIGHBOUR,
                                        float shininessOverride = -1.0f) {
        Material material{};
        material.Ka = { properties.k_a * kaScale.x, properties.k_a * kaScale.y, properties.k_a * kaScale.z };
        material.Kd = { properties.k_d * kdScale.x, properties.k_d * kdScale.y, properties.k_d * kdScale.z };
        material.Ks = { properties.k_s * ksScale.x, properties.k_s * ksScale.y, properties.k_s * ksScale.z };
        material.Ns = (shininessOverride >= 0.0f) ? shininessOverride : properties.shininess;

        if (!texturePath.empty()) {
            material.map_Kd = Texture::loadFromFile(texturePath);
            material.map_Kd.setFilter(filter);
        }

        return material;
    }

    inline int getColorFromMaterial(const float color) {
        float kaR = std::fmod(color, 1.0f);
        kaR = kaR < 0 ? 1.0f + kaR : kaR;
        return static_cast<int>(kaR * 255);
    }

    inline void setEmissiveColor(MaterialComponent& material, const slib::vec3& color) {
        for (auto& kv : material.materials) {
            kv.second.Ke = color * 255.0f;
        }
    }

} // namespace MaterialSystem
