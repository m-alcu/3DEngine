#include "worldScene.hpp"
#include "../objects/world.hpp"
#include "../objects/icosahedron.hpp"

void WorldScene::setup() {

    clearAllSolids();
    auto world = std::make_unique<World>();
    world->name = "World";
    world->setup(16, 32);

    world->position.z = -150;
    world->position.x = 0;
    world->position.y = 0;
    world->position.zoom = 70.0f;
    world->position.xAngle = 0.0f;
    world->position.yAngle = 0.0f;
    world->position.zAngle = 0.0f;
    world->incXangle = 0.25f;
    world->incYangle = 0.5f;
    world->rotationEnabled = true;
    world->shading = Shading::TexturedGouraud;
	sceneType = SceneType::WORLD;

    // Add orbiting icosahedron as point light source
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron";
    icosahedron->position.z = -150;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 0.2f;
    icosahedron->shading = Shading::Flat;
    icosahedron->lightSourceEnabled = true;
    icosahedron->light.type = LightType::Point;
    icosahedron->light.color = {1.0f, 1.0f, 1.0f};
    icosahedron->light.intensity = 4.5f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 0, -150},
        /*radius*/ 150.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(world));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
