#include "starScene.hpp"
#include "../objects/ascLoader.hpp"
#include "../objects/icosahedron.hpp"

void StarScene::setup() {

    clearAllSolids();

    auto ascLoader = std::make_unique<AscLoader>();

    ascLoader->position.z = -500;
    ascLoader->position.x = 0;
    ascLoader->position.y = 0;
    ascLoader->position.zoom = 1;
    ascLoader->position.xAngle = 0.0f;
    ascLoader->position.yAngle = 0.0f;
    ascLoader->position.zAngle = 0.0f;
    ascLoader->shading = Shading::TexturedBlinnPhong;
	sceneType = SceneType::STAR;

    ascLoader->setup("resources/STAR.ASC");

    ascLoader->enableCircularOrbit(/*center*/{ ascLoader->position.x,ascLoader->position.y,ascLoader->position.z },
        /*radius*/100.0f,
        /*planeNormal*/{ 0,0,1 },   // orbit in XZ plane
        /*omega*/(3.14159265f / 1), // 180°/s
        /*initialPhase*/0.0f);

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
        /*radius*/ 200.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(ascLoader));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
