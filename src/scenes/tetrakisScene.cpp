#include "tetrakisScene.hpp"
#include "../objects/tetrakis.hpp"
#include "../objects/icosahedron.hpp"

void TetrakisScene::setup() {

    clearAllSolids();
    auto tetrakis = std::make_unique<Tetrakis>();
    tetrakis->name = "Tetrakis";

    tetrakis->position.z = -500;
    tetrakis->position.x = 0;
    tetrakis->position.y = 0;
    tetrakis->position.zoom = 2;
    tetrakis->position.xAngle = 90.0f;
    tetrakis->position.yAngle = 0.0f;
    tetrakis->position.zAngle = 0.0f;
    tetrakis->shading = Shading::Flat;
	sceneType = SceneType::TETRAKIS;

    tetrakis->setup();

    tetrakis->enableCircularOrbit(
        { tetrakis->position.x, tetrakis->position.y, tetrakis->position.z },
        100.0f,
        { 0,1,1 },
        (3.14159265f / 3),
        0.0f
    );

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
    icosahedron->light.color = {1.0f, 0.0f, 0.0f};
    icosahedron->light.intensity = 12.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 0, -500},
        /*radius*/ 400.0f,
        /*planeNormal*/ {0, 0, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(tetrakis));
    addSolid(std::move(icosahedron));

    shadowsEnabled = false;

    Scene::setup();
}
