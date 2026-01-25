#include "knotScenePoint.hpp"
#include "../objects/ascLoader.hpp"
#include "../objects/icosahedron.hpp"

void KnotScenePoint::setup() {

  camera.pos = {25.0f, 60.0f, -250.0f};

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
  ascLoader->shading = Shading::TexturedBlinnPhong;

  // Add Orbiting Icosahedron (from TorusScene) acting as light source
  auto icosahedron = std::make_unique<Icosahedron>();
  icosahedron->name = "Light Icosahedron";

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
  icosahedron->light.type = LightType::Point;
  icosahedron->light.color = {1.0f, 1.0f, 1.0f};
  icosahedron->light.intensity = 4.0f;
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

  addSolid(std::move(ascLoader));
  addSolid(std::move(icosahedron));

  sceneType = SceneType::KNOT_POINT;

  Scene::setup();
}
