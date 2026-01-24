#include "shadowTestScene.hpp"
#include "../objects/plane.hpp"

void ShadowTestScene::setup() {
  // Point light positioned above and to the side to create shadows
  // Light comming from origin towards far y and z
  light.type = LightType::Directional;
  light.color = {1.0f, 1.0f, 1.0f};
  light.intensity = 1.0f;
  light.direction = smath::normalize(slib::vec3{0, 0, 1});

  camera.pos = {0.0f, 0.0f, 0.0f};
  camera.pitch = 0.0f;
  camera.yaw = 0.0f;
  camera.roll = 0.0f;

  clearAllSolids();

  // Large floor plane (the receiver of shadows)
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
  sceneType = SceneType::SHADOWTEST;

  // Large floor plane (the receiver of shadows)
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
  sceneType = SceneType::SHADOWTEST;

  Scene::setup();
}
