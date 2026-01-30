#include "torusScene.hpp"
#include "../objects/torus.hpp"
#include "../objects/icosahedron.hpp"

void TorusScene::setup() {

    clearAllSolids();
    auto torus = std::make_unique<Torus>();
    torus->name = "Torus";
    torus->setup(20, 10, 500, 250);

    torus->position.z = -500;
    torus->position.x = 0;
    torus->position.y = 0;
    torus->position.zoom = 0.25f;
    torus->position.xAngle = 90.0f;
    torus->position.yAngle = 0.0f;
    torus->position.zAngle = 0.0f;
    torus->rotationEnabled = true;
    torus->incYangle = 0.5f; // Rotate around Y axis
    torus->incXangle = 0.2f; // Rotate around X axis
    torus->shading = Shading::BlinnPhong;
	sceneType = SceneType::TORUS;

    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron";

    icosahedron->position.z = -500;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 0.2;
    icosahedron->position.xAngle = 90.0f;
    icosahedron->position.yAngle = 0.0f;
    icosahedron->position.zAngle = 0.0f;
    icosahedron->shading = Shading::Flat;
	icosahedron->lightSourceEnabled = true;
    icosahedron->light.type = LightType::Point;
    icosahedron->light.color = {1.0f, 1.0f, 1.0f};
    icosahedron->light.intensity = 5.0f;
    icosahedron->rotationEnabled = false;

    icosahedron->setup();

    icosahedron->enableCircularOrbit(/*center*/{ icosahedron->position.x,icosahedron->position.y,icosahedron->position.z },
        /*radius*/500.0f,
        /*planeNormal*/{ 0,1,1 },   // orbit in XZ plane
        /*omega*/(3.14159265f / 3), // 60�/s
        /*initialPhase*/0.0f);

    addSolid(std::move(torus));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
