#pragma once
#include "../scene.hpp"

class TetrakisScene : public Scene {
public:
    using Scene::Scene; // Inherit constructors

    void setup() override;
};