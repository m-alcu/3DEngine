#pragma once
#include "../scene.hpp"

class ShadowPointTestScene : public Scene {
public:
  using Scene::Scene; // Inherit constructors

  void setup() override;
};
