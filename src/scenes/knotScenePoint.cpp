#include "knotScenePoint.hpp"

void KnotScenePoint::setup() {

  // Initialize as Point Light
  light.type = LightType::Point;
  light.color = {1.0f, 1.0f, 1.0f};
  light.intensity = 1.0f;
  light.position = {0, 0, 0}; // Will be updated

  clearAllSolids();

  auto ascLoader = std::make_unique<AscLoader>();
  ascLoader->setup("resources/knot.asc");

  ascLoader->position.z = -500;
  ascLoader->position.x = 0;
  ascLoader->position.y = 0;
  ascLoader->position.zoom = 1;
  ascLoader->position.xAngle = 90.0f;
  ascLoader->position.yAngle = 0.0f;
  ascLoader->position.zAngle = 0.0f;
  ascLoader->shading = Shading::Flat;

  // Add Orbiting Icosahedron (from TorusScene) acting as light source
  auto icosahedron = std::make_unique<Icosahedron>();

  // Initial position (will be overwritten by orbit)
  icosahedron->position.z = -500;
  icosahedron->position.x = 0;
  icosahedron->position.y = 0;
  icosahedron->position.zoom = 0.2f;
  icosahedron->position.xAngle = 90.0f;
  icosahedron->position.yAngle = 0.0f;
  icosahedron->position.zAngle = 0.0f;
  icosahedron->shading = Shading::Flat;
  icosahedron->lightSourceEnabled = true;
  icosahedron->rotationEnabled = false;

  icosahedron->setup();

  // Orbit around the knot (at z=-500)
  // TorusScene used radius 1000. Using same radius.
  icosahedron->enableCircularOrbit(
      /*center*/ {0, 0, -500},
      /*radius*/ 200.0f,
      /*planeNormal*/ {0, 1, 1},   // orbit in XZ plane (tilted)
      /*omega*/ (3.14159265f / 3), // 60 deg/s
      /*initialPhase*/ PI / 2.0f); // start at top);

  // Keep reference to update light position
  lightSource = icosahedron.get();

  addSolid(std::move(ascLoader));
  addSolid(std::move(icosahedron));

  sceneType = SceneType::KNOT_POINT;
}

void KnotScenePoint::update(float dt) {
  if (lightSource) {
    light.position = {lightSource->position.x, lightSource->position.y,
                      lightSource->position.z};
  }
}
