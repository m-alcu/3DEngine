#include <iostream>
#include <math.h>
#include "scene.hpp"

void Scene::torusInit() {

    clearAllSolids();
    auto torus = std::make_unique<Torus>();
    torus->setup(20, 10, 500, 250);

    torus->position.z = -1000;
    torus->position.x = 0;
    torus->position.y = 0;
    torus->position.zoom = 1.0f;
    torus->position.xAngle = 90.0f;
    torus->position.yAngle = 0.0f;
    torus->position.zAngle = 0.0f;
    torus->shading = Shading::TexturedGouraud;
    
    addSolid(std::move(torus));

}

void Scene::amigaInit() {

    clearAllSolids();
    auto amiga = std::make_unique<Amiga>();
    amiga->setup(16, 32);

    amiga->position.z = -500;
    amiga->position.x = 0;
    amiga->position.y = 0;
    amiga->position.zoom = 100.0f;
    amiga->position.xAngle = 0.0f;
    amiga->position.yAngle = 0.0f;
    amiga->position.zAngle = 0.0f;
    amiga->shading = Shading::Flat;
    
    addSolid(std::move(amiga));

}

void Scene::worldInit() {

    clearAllSolids();
    auto world = std::make_unique<World>();
    world->setup(16, 32);

    world->position.z = -150;
    world->position.x = 0;
    world->position.y = 0;
    world->position.zoom = 100.0f;
    world->position.xAngle = 0.0f;
    world->position.yAngle = 0.0f;
    world->position.zAngle = 0.0f;
    world->shading = Shading::TexturedGouraud;
    
    addSolid(std::move(world));

}

void Scene::tetrakisInit() {

    clearAllSolids();
    auto tetrakis = std::make_unique<Tetrakis>();
    tetrakis->setup();

    tetrakis->position.z = -5000;   
    tetrakis->position.x = 0;
    tetrakis->position.y = 0;
    tetrakis->position.zoom = 25;
    tetrakis->position.xAngle = 90.0f;
    tetrakis->position.yAngle = 0.0f;
    tetrakis->position.zAngle = 0.0f;
    tetrakis->shading = Shading::Flat;
    
    addSolid(std::move(tetrakis));

}

void Scene::cubeInit() {

    clearAllSolids();
    
    auto cube = std::make_unique<Cube>();
    cube->setup();
    cube->position.z = -500;
    cube->position.x = 0;
    cube->position.y = 0;
    cube->position.zoom = 20;
    cube->position.xAngle = 0.0f;
    cube->position.yAngle = 0.0f;
    cube->position.zAngle = 0.0f;
    cube->shading = Shading::TexturedFlat;
    addSolid(std::move(cube));
    

}

void Scene::knotInit() {

    clearAllSolids();
    
    auto ascLoader = std::make_unique<AscLoader>();
    ascLoader->setup("resources/knot.asc");

    ascLoader->position.z = -500;   
    ascLoader->position.x = 0;
    ascLoader->position.y = 0;
    ascLoader->position.zoom = 1;
    ascLoader->position.xAngle = 90.0f;
    ascLoader->position.yAngle = 0.0f;
    ascLoader->position.zAngle = 0.0f;
    ascLoader->shading = Shading::Flat;
    
    addSolid(std::move(ascLoader));

}

void Scene::starInit() {

    clearAllSolids();
    
    auto ascLoader = std::make_unique<AscLoader>();
    ascLoader->setup("resources/STAR.ASC");

    ascLoader->position.z = -500;   
    ascLoader->position.x = 0;
    ascLoader->position.y = 0;
    ascLoader->position.zoom = 1;
    ascLoader->position.xAngle = 90.0f;
    ascLoader->position.yAngle = 0.0f;
    ascLoader->position.zAngle = 0.0f;
    ascLoader->shading = Shading::Flat;
    
    addSolid(std::move(ascLoader));

}




void Scene::setup() {

    // Light comming from origin towards far y and z
    lux = smath::normalize(slib::vec3{ 0, 1, 1 });
    // Far is positive z, so we are looking from the origin to the positive z axis.
    eye = { 0, 0, 1 };
	// Used in BlinnPhong shading
    halfwayVector = smath::normalize(lux + eye);

    switch (sceneType) {
        case SceneType::TORUS:
            torusInit();
            break;
        case SceneType::TETRAKIS:
            tetrakisInit();
            break;
        case SceneType::CUBE:
            cubeInit();
            break;
        case SceneType::KNOT:
            knotInit();
            break;   
        case SceneType::STAR:
            starInit();
            break;    
        case SceneType::AMIGA:
            amigaInit();
            break;    
        case SceneType::WORLD:
            worldInit();
            break; 
        default:
            tetrakisInit();
            break;
    }

}
