#pragma once
#include "../scene.hpp"

class KnotScenePoint : public Scene {
  Icosahedron *lightSource = nullptr;

public:
  using Scene::Scene; // Inherit constructors

  void setup() override;
  void update(float dt) override;
};
