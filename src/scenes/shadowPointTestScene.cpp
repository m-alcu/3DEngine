#include "shadowPointTestScene.hpp"
#include "../objects/plane.hpp"

void ShadowPointTestScene::setup() {

  camera.pos = {0.0f, 0.0f, 0.0f};
  camera.pitch = 0.0f;
  camera.yaw = 0.0f;
  camera.roll = 0.0f;

  // Initialize as Point Light at camera position
  light.type = LightType::Point;
  light.color = {1.0f, 1.0f, 1.0f};
  light.intensity = 1.0f;
  light.position = camera.pos; // Will be updated in update()

  clearAllSolids();

  // Large floor plane (the receiver of shadows) - from shadowTestScene
  auto floor = std::make_unique<Plane>(150.f);
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

  sceneType = SceneType::SHADOWTEST_POINT;
}

void ShadowPointTestScene::update(float dt) {
  // Update light position to follow camera
  //light.position = camera.pos;
}
