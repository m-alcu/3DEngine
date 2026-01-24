#pragma once
#include "../scene.hpp"

class CubeScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
