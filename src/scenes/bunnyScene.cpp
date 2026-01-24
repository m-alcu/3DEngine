#include "bunnyScene.hpp"
#include "../objects/objLoader.hpp"
#include "../objects/icosahedron.hpp"

void BunnyScene::setup() {

    clearAllSolids();

    auto bunny = std::make_unique<ObjLoader>();
    bunny->setup("resources/bunny.obj");

    bunny->position.z = -500;
    bunny->position.x = 100;
    bunny->position.y = -250;
    bunny->position.xAngle = -10.0f;
    bunny->position.yAngle = 0.0f;
    bunny->position.zAngle = 0.0f;
    bunny->shading = Shading::TexturedPhong;
    sceneType = SceneType::BUNNY;

    // Add orbiting icosahedron as point light source
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron";
    icosahedron->position.z = -500;
    icosahedron->position.x = 100;
    icosahedron->position.y = -250;
    icosahedron->position.zoom = 0.2f;
    icosahedron->shading = Shading::Flat;
    icosahedron->lightSourceEnabled = true;
    icosahedron->light.type = LightType::Point;
    icosahedron->light.color = {1.0f, 1.0f, 1.0f};
    icosahedron->light.intensity = 1.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {100, -250, -500},
        /*radius*/ 200.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(bunny));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
