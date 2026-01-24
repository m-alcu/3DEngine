#pragma once
#include "../scene.hpp"

class WorldScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
