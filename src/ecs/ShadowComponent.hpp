#pragma once
#include <cstdint>
#include <memory>
#include "../ShadowMap.hpp"

struct ShadowComponent {
    std::shared_ptr<ShadowMap> shadowMap;
};
