#include "shadowPointTestScene.hpp"
#include "../objects/plane.hpp"
#include "../objects/icosahedron.hpp"

void ShadowPointTestScene::setup() {

  camera.pos = {50.0f, 100.0f, 100.0f};
  camera.pitch = 0.0f;
  camera.yaw = 0.0f;
  camera.roll = 0.0f;

  clearAllSolids();

  // Large floor plane (the receiver of shadows) - from shadowTestScene
  auto floor = std::make_unique<Plane>(150.f);
  floor->name = "Floor Plane";
  floor->setup();
  floor->position.z = -500;
  floor->position.x = 0;
  floor->position.y = 0;
  floor->position.zoom = 1;
  floor->position.xAngle = 15.0f;
  floor->position.yAngle = 0.0f;
  floor->position.zAngle = 0.0f;
  floor->shading = Shading::Flat;
  floor->rotationEnabled = false;
  addSolid(std::move(floor));

  // Front plane (shadow caster) - from shadowTestScene
  auto front = std::make_unique<Plane>(50.f);
  front->name = "Front Plane";
  front->setup();
  front->position.z = -300;
  front->position.x = 0;
  front->position.y = 0;
  front->position.zoom = 1;
  front->position.xAngle = 0.0f;
  front->position.yAngle = 0.0f;
  front->position.zAngle = 0.0f;
  front->shading = Shading::Flat;
  front->rotationEnabled = true;
  addSolid(std::move(front));

  // Add orbiting icosahedron as point light source
  auto icosahedron = std::make_unique<Icosahedron>();
  icosahedron->name = "Light Icosahedron";
  icosahedron->position.z = -400;
  icosahedron->position.x = 0;
  icosahedron->position.y = 0;
  icosahedron->position.zoom = 0.2f;
  icosahedron->shading = Shading::Flat;
  icosahedron->lightSourceEnabled = true;
  icosahedron->light.type = LightType::Point;
  icosahedron->light.intensity = 4.0f;
  icosahedron->rotationEnabled = false;
  icosahedron->setup();
  icosahedron->light.color = {1.0f, 0.0f, 0.0f};
  icosahedron->setEmissiveColor({1.0f, 0.0f, 0.0f});
  icosahedron->enableCircularOrbit(
      /*center*/ {0, 0, -400},
      /*radius*/ 266.0f,
      /*planeNormal*/ {1, 0, 0},
      /*omega*/ (3.14159265f / 3),
      /*initialPhase*/ 0.0f);
  addSolid(std::move(icosahedron));

  sceneType = SceneType::SHADOWTEST_POINT;

  Scene::setup();
}
