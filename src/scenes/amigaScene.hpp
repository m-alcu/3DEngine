#pragma once
#include "../scene.hpp"

class AmigaScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};
