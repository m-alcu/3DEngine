#pragma once
#include "../scene.hpp"

class IcosahedronScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
