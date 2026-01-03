#include "torusScene.hpp"

void TorusScene::setup() {

    // Light comming from origin towards far y and z
    light.type = LightType::Directional;
    light.color = { 1.0f, 1.0f, 1.0f };
    light.intensity = 1.0f;
    light.direction = smath::normalize(slib::vec3{ 1, 1, 1 });

    clearAllSolids();
    auto torus = std::make_unique<Torus>();
    torus->setup(20, 10, 500, 250);

    torus->position.z = -1000;
    torus->position.x = 0;
    torus->position.y = 0;
    torus->position.zoom = 0.5f;
    torus->position.xAngle = 90.0f;
    torus->position.yAngle = 0.0f;
    torus->position.zAngle = 0.0f;
    torus->shading = Shading::BlinnPhong;
	sceneType = SceneType::TORUS;

    auto icosahedron = std::make_unique<Icosahedron>();

    icosahedron->position.z = -1000;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 0.2;
    icosahedron->position.xAngle = 90.0f;
    icosahedron->position.yAngle = 0.0f;
    icosahedron->position.zAngle = 0.0f;
    icosahedron->shading = Shading::Flat;
	icosahedron->lightSourceEnabled = true;
    icosahedron->rotationEnabled = false;

    icosahedron->setup();

    icosahedron->enableCircularOrbit(/*center*/{ icosahedron->position.x,icosahedron->position.y,icosahedron->position.z },
        /*radius*/1000.0f,
        /*planeNormal*/{ 0,1,1 },   // orbit in XZ plane
        /*omega*/(3.14159265f / 3), // 60°/s
        /*initialPhase*/0.0f);

    addSolid(std::move(torus));
    addSolid(std::move(icosahedron));
}