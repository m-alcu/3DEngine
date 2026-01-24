#include "icosahedronScene.hpp"
#include "../objects/icosahedron.hpp"

void IcosahedronScene::setup() {

    clearAllSolids();
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Icosahedron";

    icosahedron->position.z = -5000;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 25;
    icosahedron->position.xAngle = 90.0f;
    icosahedron->position.yAngle = 0.0f;
    icosahedron->position.zAngle = 0.0f;
    icosahedron->shading = Shading::Flat;
	sceneType = SceneType::ICOSAHEDRON;

    icosahedron->setup();

    icosahedron->enableCircularOrbit(/*center*/{ icosahedron->position.x,icosahedron->position.y,icosahedron->position.z },
        /*radius*/1000.0f,
        /*planeNormal*/{ 0,1,1 },   // orbit in XZ plane
        /*omega*/(3.14159265f / 3), // 60°/s
        /*initialPhase*/0.0f);

    // Add orbiting icosahedron as point light source
    auto lightIco = std::make_unique<Icosahedron>();
    lightIco->name = "Light Icosahedron";
    lightIco->position.z = -5000;
    lightIco->position.x = 0;
    lightIco->position.y = 0;
    lightIco->position.zoom = 0.2f;
    lightIco->shading = Shading::Flat;
    lightIco->lightSourceEnabled = true;
    lightIco->light.type = LightType::Point;
    lightIco->light.color = {1.0f, 1.0f, 1.0f};
    lightIco->light.intensity = 1.0f;
    lightIco->rotationEnabled = false;
    lightIco->setup();
    lightIco->enableCircularOrbit(
        /*center*/ {0, 0, -5000},
        /*radius*/ 1500.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(icosahedron));
    addSolid(std::move(lightIco));

    Scene::setup();
}
