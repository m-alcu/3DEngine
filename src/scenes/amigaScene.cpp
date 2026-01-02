#include "amigaScene.hpp"

void AmigaScene::setup() {

    // Light comming from origin towards far y and z
    light.type = LightType::Directional;
    light.color = { 1.0f, 1.0f, 1.0f };
    light.intensity = 1.0f;
    light.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

    clearAllSolids();
    auto amiga = std::make_unique<Amiga>();
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

    addSolid(std::move(amiga));
}