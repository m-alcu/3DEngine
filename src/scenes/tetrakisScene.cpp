#include "tetrakisScene.hpp"
#include "../objects/tetrakis.hpp"
#include "../objects/icosahedron.hpp"

void TetrakisScene::setup() {

    clearAllSolids();
    auto tetrakis = std::make_unique<Tetrakis>();
    tetrakis->name = "Tetrakis";

    tetrakis->position.z = -5000;
    tetrakis->position.x = 0;
    tetrakis->position.y = 0;
    tetrakis->position.zoom = 25;
    tetrakis->position.xAngle = 90.0f;
    tetrakis->position.yAngle = 0.0f;
    tetrakis->position.zAngle = 0.0f;
    tetrakis->shading = Shading::Flat;
	sceneType = SceneType::TETRAKIS;

    tetrakis->setup();

    tetrakis->enableCircularOrbit(
        { tetrakis->position.x, tetrakis->position.y, tetrakis->position.z },
        1000.0f,
        { 0,1,1 },
        (3.14159265f / 3),
        0.0f
    );

    // Add orbiting icosahedron as point light source
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron";
    icosahedron->position.z = -5000;
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
        /*center*/ {0, 0, -5000},
        /*radius*/ 1500.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(tetrakis));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
