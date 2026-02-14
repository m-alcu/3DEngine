#pragma once
#include <cstdint>
#include <memory>
#include "../shadow_map.hpp"

struct ShadowComponent {
    std::shared_ptr<ShadowMap> shadowMap;
};
