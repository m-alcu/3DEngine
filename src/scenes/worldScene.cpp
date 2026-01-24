#include "worldScene.hpp"
#include "../objects/world.hpp"

void WorldScene::setup() {

    // Light comming from origin towards far y and z
    defaultLight.type = LightType::Directional;
    defaultLight.color = { 1.0f, 1.0f, 1.0f };
    defaultLight.intensity = 1.0f;
    defaultLight.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

    clearAllSolids();
    auto world = std::make_unique<World>();
    world->name = "World";
    world->setup(16, 32);

    world->position.z = -150;
    world->position.x = 0;
    world->position.y = 0;
    world->position.zoom = 100.0f;
    world->position.xAngle = 0.0f;
    world->position.yAngle = 0.0f;
    world->position.zAngle = 0.0f;
    world->shading = Shading::TexturedGouraud;
	sceneType = SceneType::WORLD;

    addSolid(std::move(world));

    Scene::setup();
}
