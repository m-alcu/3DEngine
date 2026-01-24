#pragma once
#include "../scene.hpp"

class TorusScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
