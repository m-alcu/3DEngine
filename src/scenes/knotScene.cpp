#include "knotScene.hpp"
#include "../objects/ascLoader.hpp"

void KnotScene::setup() {

    // Light comming from origin towards far y and z
    light.type = LightType::Directional;
    light.color = { 1.0f, 1.0f, 1.0f };
    light.intensity = 1.0f;
    light.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

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
	sceneType = SceneType::KNOT;

    addSolid(std::move(ascLoader));

    Scene::setup();
}