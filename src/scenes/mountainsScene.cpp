#include "mountainsScene.hpp"
#include "../objects/objLoader.hpp"
#include "../objects/icosahedron.hpp"

void MountainsScene::setup() {

    clearAllSolids();

    auto mountains = std::make_unique<ObjLoader>();
    mountains->setup("resources/mountains.obj");

    mountains->position.z = -500;
    mountains->position.x = 0;
    mountains->position.y = 0;
    mountains->position.xAngle = 0.0f;
    mountains->position.yAngle = 0.0f;
    mountains->position.zAngle = 0.0f;
    mountains->shading = Shading::Gouraud;
    sceneType = SceneType::MOUNTAINS;

    // Add orbiting icosahedron as point light source
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron";
    icosahedron->position.z = -500;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 0.2f;
    icosahedron->shading = Shading::Flat;
    icosahedron->lightSourceEnabled = true;
    icosahedron->light.type = LightType::Point;
    icosahedron->light.color = {1.0f, 1.0f, 1.0f};
    icosahedron->light.intensity = 90.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 200, -500},
        /*radius*/ 500.0f,
        /*planeNormal*/ {1, 1, 1},
        /*omega*/ (3.14159265f / 30.0f),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(mountains));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
