#pragma once
#include <vector>
#include "texture.hpp"
#include "slib.hpp"

class Material {
public:
    float Ns{};
    slib::vec3 Ka{};
    slib::vec3 Kd{};
    slib::vec3 Ks{};
    slib::vec3 Ke{};
    float Ni{};
    float d{};
    int illum{};
    Texture map_Kd;
    Texture map_Ks;
    Texture map_Ns;
};
