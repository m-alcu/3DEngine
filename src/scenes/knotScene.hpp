#pragma once
#include "../scene.hpp"

class KnotScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
