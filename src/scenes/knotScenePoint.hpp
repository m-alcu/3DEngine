#pragma once
#include "../scene.hpp"

class KnotScenePoint : public Scene {
public:
  using Scene::Scene; // Inherit constructors

  void setup() override;
};
