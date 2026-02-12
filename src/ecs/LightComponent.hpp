#pragma once
#include "../light.hpp"
#include "../ShadowMap.hpp"
#include <memory>

struct LightComponent {
    Light light;
    std::shared_ptr<ShadowMap> shadowMap;
};
