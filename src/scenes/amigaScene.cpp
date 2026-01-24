#include "amigaScene.hpp"
#include "../objects/amiga.hpp"
#include "../objects/icosahedron.hpp"

void AmigaScene::setup() {

    clearAllSolids();
    auto amiga = std::make_unique<Amiga>();
    amiga->name = "Amiga";
    amiga->setup(16, 32);

    amiga->position.z = -500;
    amiga->position.x = 0;
    amiga->position.y = 0;
    amiga->position.zoom = 100.0f;
    amiga->position.xAngle = 0.0f;
    amiga->position.yAngle = 0.0f;
    amiga->position.zAngle = 0.0f;
    amiga->shading = Shading::Flat;
	sceneType = SceneType::AMIGA;

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
    icosahedron->light.intensity = 1.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 0, -500},
        /*radius*/ 150.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(amiga));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
