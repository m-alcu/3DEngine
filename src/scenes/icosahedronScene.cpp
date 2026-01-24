#include "icosahedronScene.hpp"
#include "../objects/icosahedron.hpp"

void IcosahedronScene::setup() {

    // Light comming from origin towards far y and z
    defaultLight.type = LightType::Directional;
    defaultLight.color = { 1.0f, 1.0f, 1.0f };
    defaultLight.intensity = 1.0f;
    defaultLight.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

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
        /*omega*/(3.14159265f / 3), // 60�/s
        /*initialPhase*/0.0f);

    addSolid(std::move(icosahedron));

    Scene::setup();
}
