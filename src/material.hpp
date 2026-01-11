#pragma once
#include <vector>
#include "texture.hpp"
#include "slib.hpp"

namespace slib {

class Material {
public:
    float Ns{};
    vec3 Ka{};
    vec3 Kd{};
    vec3 Ks{};
    vec3 Ke{};
    float Ni{};
    float d{};
    int illum{};
    Texture map_Kd;
    Texture map_Ks;
    Texture map_Ns;
};

} // namespace slib
