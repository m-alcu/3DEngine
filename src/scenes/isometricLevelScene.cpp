#include "isometricLevelScene.hpp"
#include "../objects/objLoader.hpp"
#include "../objects/icosahedron.hpp"

void IsometricLevelScene::setup() {

    clearAllSolids();

    auto isometricLevel = std::make_unique<ObjLoader>();
    isometricLevel->setup("resources/Isometric_Game_Level_Low_Poly.obj");

    isometricLevel->position.z = -500;
    isometricLevel->position.x = 0;
    isometricLevel->position.y = 0;
    isometricLevel->position.xAngle = 0.0f;
    isometricLevel->position.yAngle = 0.0f;
    isometricLevel->position.zAngle = 0.0f;
    isometricLevel->shading = Shading::Gouraud;
    sceneType = SceneType::ISOMETRIC_LEVEL;

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
    icosahedron->light.intensity = 72.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 200, -500},
        /*radius*/ 500.0f,
        /*planeNormal*/ {1, 1, 1},
        /*omega*/ (3.14159265f / 30.0f),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(isometricLevel));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
