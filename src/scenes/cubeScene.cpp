#include "cubeScene.hpp"
#include "../objects/cube.hpp"

void CubeScene::setup() {

    // Light comming from origin towards far y and z
    light.type = LightType::Point;
    light.color = { 1.0f, 1.0f, 1.0f };
    light.intensity = 10.0f;

    shadowsEnabled = false; 

    clearAllSolids();

    auto cube = std::make_unique<Cube>();
    cube->name = "Cube";
    cube->setup();
    cube->position.z = -500;
    cube->position.x = 0;
    cube->position.y = 0;
    cube->position.zoom = 20;
    cube->position.xAngle = 0.0f;
    cube->position.yAngle = 0.0f;
    cube->position.zAngle = 0.0f;
    cube->shading = Shading::TexturedFlat;
	sceneType = SceneType::CUBE;
    addSolid(std::move(cube));

    Scene::setup();
}
