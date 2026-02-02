#include "vikingRoomScene.hpp"
#include "../objects/objLoader.hpp"
#include "../objects/icosahedron.hpp"

void VikingRoomScene::setup() {

    clearAllSolids();

    auto vikingRoom = std::make_unique<ObjLoader>();
    vikingRoom->setup("resources/viking_room.obj");

    vikingRoom->position.z = -500;
    vikingRoom->position.x = 0;
    vikingRoom->position.y = 0;
    vikingRoom->position.xAngle = 0.0f;
    vikingRoom->position.yAngle = 0.0f;
    vikingRoom->position.zAngle = 0.0f;
    vikingRoom->shading = Shading::TexturedPhong;
    sceneType = SceneType::VIKING_ROOM;

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
    icosahedron->light.intensity = 8.0f;
    icosahedron->rotationEnabled = false;
    icosahedron->setup();
    icosahedron->enableCircularOrbit(
        /*center*/ {0, 200, -500},
        /*radius*/ 500.0f,
        /*planeNormal*/ {1, 1, 1},
        /*omega*/ (3.14159265f / 30.0f),
        /*initialPhase*/ 0.0f);

    addSolid(std::move(vikingRoom));
    addSolid(std::move(icosahedron));

    Scene::setup();
}
