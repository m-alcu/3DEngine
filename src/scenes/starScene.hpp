#pragma once
#include "../scene.hpp"

class StarScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
