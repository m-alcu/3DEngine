#include "sponzaScene.hpp"
#include "../objects/objLoader.hpp"
#include "../objects/icosahedron.hpp"

void SponzaScene::setup() {

    clearAllSolids();

    auto sponza = std::make_unique<ObjLoader>();
    sponza->setup("resources/sponza.obj");

    sponza->position.z = -500;
    sponza->position.x = 0;
    sponza->position.y = 0;
    sponza->position.xAngle = 0.0f;
    sponza->position.yAngle = 0.0f;
    sponza->position.zAngle = 0.0f;
    sponza->shading = Shading::TexturedPhong;
    sceneType = SceneType::SPONZA;
    shadowsEnabled = false;

    camera.pos = {0.0f, 50.0f, -500.0f};
    camera.forward = {1.0f, 0.0f, 0.0f};

    // Add orbiting icosahedron as point light source
    auto icosahedron = std::make_unique<Icosahedron>();
    icosahedron->name = "Light Icosahedron";
    icosahedron->position.z = -500;
    icosahedron->position.x = 0;
    icosahedron->position.y = 0;
    icosahedron->position.zoom = 0.02f;
    icosahedron->shading = Shading::Flat;
    icosahedron->lightSourceEnabled = true;
    icosahedron->light.type = LightType::Point;
    icosahedron->light.color = {1.0f, 1.0f, 1.0f};
    icosahedron->light.intensity = 2.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 200, -500},
        /*radius*/ 40.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(sponza));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
