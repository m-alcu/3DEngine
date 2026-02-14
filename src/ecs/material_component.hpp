#pragma once
#include <map>
#include <string>
#include "../material.hpp"

struct MaterialComponent {
    std::map<std::string, Material> materials;
};
