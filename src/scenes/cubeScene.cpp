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
    cube->rotationEnabled = true;
    cube->incYangle = 0.3f; // Rotate around Y axis
    cube->incXangle = 0.2f; // Rotate around X axis
    cube->shading = Shading::TexturedFlat;
	sceneType = SceneType::CUBE;

    // Add orbiting icosahedron as point light source
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron green";
    icosahedron->position.z = -500;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 0.2f;
    icosahedron->shading = Shading::Flat;
    icosahedron->lightSourceEnabled = true;
    icosahedron->light.type = LightType::Point;
    icosahedron->light.intensity = 3.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->light.color = {0.5f, 1.0f, 0.5f};
    icosahedron->setEmissiveColor({0.5f, 1.0f, 0.5f});
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 0, -500},
        /*radius*/ 500.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(cube));
    addSolid(std::move(icosahedron));

    // Add orbiting icosahedron as point light source
    auto icosahedron2 = std::make_unique<Icosahedron>();
    icosahedron2->name = "Light Icosahedron blue";
    icosahedron2->position.z = -500;
    icosahedron2->position.x = 0;
    icosahedron2->position.y = 0;
    icosahedron2->position.zoom = 0.2f;
    icosahedron2->shading = Shading::Flat;
    icosahedron2->lightSourceEnabled = true;
    icosahedron2->light.type = LightType::Point;
    icosahedron2->light.intensity = 3.0f;
    icosahedron2->rotationEnabled = false;
    icosahedron2->setup();
    icosahedron2->light.color = {0.5f, 0.5f, 1.0f};
    icosahedron2->setEmissiveColor({0.5f, 0.5f, 1.0f});
    icosahedron2->enableCircularOrbit(
        /*center*/ {0, 0, -500},
        /*radius*/ 500.0f,
        /*planeNormal*/ {1, 1, 0},
        /*omega*/ (-3.14159265f / 5),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(icosahedron2));

    // Add orbiting icosahedron as point light source
    auto icosahedron3 = std::make_unique<Icosahedron>();
    icosahedron3->name = "Light Icosahedron red";
    icosahedron3->position.z = -500;
    icosahedron3->position.x = 0;
    icosahedron3->position.y = 0;
    icosahedron3->position.zoom = 0.2f;
    icosahedron3->shading = Shading::Flat;
    icosahedron3->lightSourceEnabled = true;
    icosahedron3->light.type = LightType::Point;
    icosahedron3->light.intensity = 3.0f;
    icosahedron3->rotationEnabled = false;
    icosahedron3->setup();
    icosahedron3->light.color = {1.0f, 0.5f, 0.5f};
    icosahedron3->setEmissiveColor({1.0f, 0.5f, 0.5f});
    icosahedron3->enableCircularOrbit(
        /*center*/ {0, 0, -500},
        /*radius*/ 500.0f,
        /*planeNormal*/ {1, 0, 1},
        /*omega*/ (3.14159265f / 2),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(icosahedron3));    


    Scene::setup();
}
