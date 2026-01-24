#include "cubeScene.hpp"
#include "../objects/cube.hpp"
#include "../objects/icosahedron.hpp"

void CubeScene::setup() {

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
    icosahedron->light.intensity = 10.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 0, -500},
        /*radius*/ 350.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(cube));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
