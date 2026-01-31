#include "suzanneScene.hpp"
#include "../objects/objLoader.hpp"
#include "../objects/icosahedron.hpp"

void SuzanneScene::setup() {

    clearAllSolids();

    auto suzanne = std::make_unique<ObjLoader>();
    suzanne->setup("resources/suzanne.obj");

    suzanne->position.z = -500;
    suzanne->position.x = 0;
    suzanne->position.y = 0;
    suzanne->position.xAngle = 0.0f;
    suzanne->position.yAngle = 0.0f;
    suzanne->position.zAngle = 0.0f;
    suzanne->shading = Shading::TexturedPhong;
    sceneType = SceneType::SUZANNE;

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
    icosahedron->light.intensity = 2.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 200, -500},
        /*radius*/ 400.0f,
        /*planeNormal*/ {0, 1, 1},
        /*omega*/ (3.14159265f / 3),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(suzanne));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
