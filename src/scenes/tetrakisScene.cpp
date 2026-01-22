#include "tetrakisScene.hpp"
#include "../objects/tetrakis.hpp"

void TetrakisScene::setup() {

    // Light comming from origin towards far y and z
    light.type = LightType::Directional;
    light.color = { 1.0f, 1.0f, 1.0f };
    light.intensity = 1.0f;
    light.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

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

    addSolid(std::move(tetrakis));
}
