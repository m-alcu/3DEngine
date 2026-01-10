#pragma once
#include "../scene.hpp"

class ShadowTestScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
