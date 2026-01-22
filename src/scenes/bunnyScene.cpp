#include "bunnyScene.hpp"
#include "../objects/objLoader.hpp"

void BunnyScene::setup() {

    light.type = LightType::Directional;
    light.color = { 1.0f, 1.0f, 1.0f };
    light.intensity = 1.0f;
    light.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

    clearAllSolids();

    auto bunny = std::make_unique<ObjLoader>();
    bunny->setup("resources/bunny.obj");

    bunny->position.z = -500;
    bunny->position.x = 0;
    bunny->position.y = 0;
    bunny->position.xAngle = 0.0f;
    bunny->position.yAngle = 0.0f;
    bunny->position.zAngle = 0.0f;
    bunny->shading = Shading::TexturedPhong;
    sceneType = SceneType::BUNNY;

    addSolid(std::move(bunny));
}
