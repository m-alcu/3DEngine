#include "scene_loader.hpp"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <stdexcept>
#include <unordered_map>

#include "../backgrounds/skybox.hpp"
#include "../backgrounds/background_factory.hpp"
#include "../ecs/material_system.hpp"
#include "../ecs/mesh_system.hpp"
#include "../ecs/prefab_factory.hpp"
#include "../ecs/name_component.hpp"

// ---------------------------------------------------------------------------
// Enum parsers
// ---------------------------------------------------------------------------

Shading SceneLoader::parseShading(const std::string& str) {
    static const std::unordered_map<std::string, Shading> map = {
        {"wireframe",            Shading::Wireframe},
        {"flat",                 Shading::Flat},
        {"gouraud",              Shading::Gouraud},
        {"blinn_phong",          Shading::BlinnPhong},
        {"phong",                Shading::Phong},
        {"textured_flat",        Shading::TexturedFlat},
        {"textured_gouraud",     Shading::TexturedGouraud},
        {"textured_blinn_phong", Shading::TexturedBlinnPhong},
        {"textured_phong",       Shading::TexturedPhong},
    };
    auto it = map.find(str);
    if (it == map.end())
        throw std::runtime_error("Unknown shading mode: " + str);
    return it->second;
}

LightType SceneLoader::parseLightType(const std::string& str) {
    static const std::unordered_map<std::string, LightType> map = {
        {"directional", LightType::Directional},
        {"point",       LightType::Point},
        {"spot",        LightType::Spot},
    };
    auto it = map.find(str);
    if (it == map.end())
        throw std::runtime_error("Unknown light type: " + str);
    return it->second;
}

BackgroundType SceneLoader::parseBackgroundType(const std::string& str) {
    static const std::unordered_map<std::string, BackgroundType> map = {
        {"desert",    BackgroundType::DESERT},
        {"image_png", BackgroundType::IMAGE_PNG},
        {"twister",   BackgroundType::TWISTER},
        {"skybox",        BackgroundType::SKYBOX},
        {"hdr_panorama",  BackgroundType::HDR_PANORAMA},
    };
    auto it = map.find(str);
    if (it == map.end())
        throw std::runtime_error("Unknown background type: " + str);
    return it->second;
}

// ---------------------------------------------------------------------------
// Sub-parsers
// ---------------------------------------------------------------------------

void SceneLoader::parseCamera(const YAML::Node& node, Camera& camera) {
    if (node["position"]) {
        auto p = node["position"];
        camera.pos = {p[0].as<float>(), p[1].as<float>(), p[2].as<float>()};
    }
    if (node["pitch"])       camera.pitch       = node["pitch"].as<float>();
    if (node["yaw"])         camera.yaw         = node["yaw"].as<float>();
    if (node["roll"])        camera.roll        = node["roll"].as<float>();
    if (node["forward"]) {
        auto f = node["forward"];
        camera.forward = {f[0].as<float>(), f[1].as<float>(), f[2].as<float>()};
    }
    if (node["z_near"])      camera.zNear       = node["z_near"].as<float>();
    if (node["z_far"])       camera.zFar        = node["z_far"].as<float>();
    if (node["view_angle"])  camera.viewAngle   = node["view_angle"].as<float>();
    if (node["speed"])       camera.speed       = node["speed"].as<float>();
    if (node["eagerness"])   camera.eagerness   = node["eagerness"].as<float>();
    if (node["sensitivity"]) camera.sensitivity = node["sensitivity"].as<float>();
}

void SceneLoader::parseLight(const YAML::Node& node, Light& light) {
    if (node["type"])
        light.type = parseLightType(node["type"].as<std::string>());
    if (node["color"]) {
        auto c = node["color"];
        light.color = {c[0].as<float>(), c[1].as<float>(), c[2].as<float>()};
    }
    if (node["intensity"])
        light.intensity = node["intensity"].as<float>();
    if (node["direction"]) {
        auto d = node["direction"];
        light.direction = {d[0].as<float>(), d[1].as<float>(), d[2].as<float>()};
    }
    if (node["radius"])
        light.radius = node["radius"].as<float>();
    if (node["inner_cutoff"])
        light.innerCutoff = node["inner_cutoff"].as<float>();
    if (node["outer_cutoff"])
        light.outerCutoff = node["outer_cutoff"].as<float>();
}

void SceneLoader::parseOrbit(const YAML::Node& node, TransformComponent& transform) {
    slib::vec3 center{0, 0, 0};
    float radius = 1.0f;
    slib::vec3 planeNormal{0, 1, 0};
    float omega = 1.0f;
    float initialPhase = 0.0f;

    if (node["center"]) {
        auto c = node["center"];
        center = {c[0].as<float>(), c[1].as<float>(), c[2].as<float>()};
    }
    if (node["radius"])
        radius = node["radius"].as<float>();
    if (node["plane_normal"]) {
        auto n = node["plane_normal"];
        planeNormal = {n[0].as<float>(), n[1].as<float>(), n[2].as<float>()};
    }
    if (node["omega"])
        omega = node["omega"].as<float>();
    if (node["initial_phase"])
        initialPhase = node["initial_phase"].as<float>();

    TransformSystem::enableCircularOrbit(transform, center, radius, planeNormal, omega, initialPhase);
}

void SceneLoader::parsePosition(const YAML::Node& node, TransformComponent& transform) {
    if (node["position"]) {
        auto p = node["position"];
        transform.position.x = p[0].as<float>();
        transform.position.y = p[1].as<float>();
        transform.position.z = p[2].as<float>();
    }
    if (node["angles"]) {
        auto a = node["angles"];
        transform.position.xAngle = a[0].as<float>();
        transform.position.yAngle = a[1].as<float>();
        transform.position.zAngle = a[2].as<float>();
    }
    if (node["zoom"])
        transform.position.zoom = node["zoom"].as<float>();
}

// ---------------------------------------------------------------------------
// Entity factory
// ---------------------------------------------------------------------------

Entity SceneLoader::parseEntity(const YAML::Node& node, Scene& scene) {
    std::string type = node["type"].as<std::string>();
    Entity entity = scene.createEntity();

    TransformComponent transform{};
    MeshComponent mesh{};
    MaterialComponent material{};
    RenderComponent render{};
    RotationComponent rotation{};
    NameComponent name{};
    bool isLight = false;

    if (type == "obj_loader") {
        std::string file = node["file"].as<std::string>();
        PrefabFactory::buildObj(file, mesh, material, transform);
        if (!node["name"]) {
            std::filesystem::path filePath(file);
            name.name = filePath.stem().string();
        }
    } else if (type == "asc_loader") {
        std::string file = node["file"].as<std::string>();
        PrefabFactory::buildAsc(file, mesh, material);
        if (!node["name"]) {
            std::filesystem::path filePath(file);
            name.name = filePath.stem().string();
        }
    } else if (type == "cube") {
        PrefabFactory::buildCube(mesh, material);
    } else if (type == "icosahedron") {
        PrefabFactory::buildIcosahedron(mesh, material);
    } else if (type == "tetrakis") {
        PrefabFactory::buildTetrakis(mesh, material);
    } else if (type == "torus") {
        int uSteps = node["u_steps"] ? node["u_steps"].as<int>() : 20;
        int vSteps = node["v_steps"] ? node["v_steps"].as<int>() : 10;
        float R    = node["major_radius"] ? node["major_radius"].as<float>() : 500.0f;
        float r    = node["minor_radius"] ? node["minor_radius"].as<float>() : 250.0f;
        PrefabFactory::buildTorus(mesh, material, uSteps, vSteps, R, r);
    } else if (type == "plane") {
        float size = node["size"] ? node["size"].as<float>() : 10.0f;
        PrefabFactory::buildPlane(mesh, material, size);
    } else if (type == "world") {
        int lat = node["latitude"]  ? node["latitude"].as<int>()  : 16;
        int lon = node["longitude"] ? node["longitude"].as<int>() : 32;
        PrefabFactory::buildWorld(mesh, material, lat, lon);
    } else if (type == "amiga") {
        int lat = node["latitude"]  ? node["latitude"].as<int>()  : 16;
        int lon = node["longitude"] ? node["longitude"].as<int>() : 32;
        PrefabFactory::buildAmiga(mesh, material, lat, lon);
    } else if (type == "test") {
        PrefabFactory::buildTest(mesh, material);
    } else {
        throw std::runtime_error("Unknown solid type: " + type);
    }

    if (node["name"]) {
        name.name = node["name"].as<std::string>();
    }

    parsePosition(node, transform);

    if (node["shading"]) {
        render.shading = parseShading(node["shading"].as<std::string>());
    }

    if (node["rotation_enabled"]) {
        rotation.enabled = node["rotation_enabled"].as<bool>();
    }

    if (node["rotation_speed"]) {
        auto rs = node["rotation_speed"];
        rotation.incXangle = rs[0].as<float>();
        rotation.incYangle = rs[1].as<float>();
    }

    if (node["light"]) {
        LightComponent lc;
        parseLight(node["light"], lc.light);
        scene.registry.lights().add(entity, std::move(lc));
        scene.registry.shadows().add(entity, ShadowComponent{});
        isLight = true;
    }

    if (node["emissive_color"]) {
        auto ec = node["emissive_color"];
        MaterialSystem::setEmissiveColor(material,
                                         {ec[0].as<float>(),
                                          ec[1].as<float>(),
                                          ec[2].as<float>()});
    }

    if (node["orbit"]) {
        parseOrbit(node["orbit"], transform);
    }

    scene.registry.transforms().add(entity, std::move(transform));
    scene.registry.meshes().add(entity, std::move(mesh));
    MeshSystem::markBoundsDirty(*scene.registry.meshes().get(entity));
    scene.registry.materials().add(entity, std::move(material));
    scene.registry.renders().add(entity, std::move(render));
    scene.registry.names().add(entity, std::move(name));

    if (!isLight) {
        scene.registry.rotations().add(entity, std::move(rotation));
    }

    return entity;
}

// ---------------------------------------------------------------------------
// Main entry point
// ---------------------------------------------------------------------------

std::unique_ptr<Scene> SceneLoader::loadFromFile(const std::string& yamlPath,
                                                  Screen scr) {
    YAML::Node root;
    try {
        root = YAML::LoadFile(yamlPath);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to load YAML file '" + yamlPath +
                                 "': " + e.what());
    }

    if (!root["scene"])
        throw std::runtime_error("YAML missing top-level 'scene' key in " +
                                 yamlPath);

    YAML::Node sceneNode = root["scene"];

    auto scene = std::make_unique<Scene>(scr);
    scene->sceneType = SceneType::YAML;

    // Scene name
    if (sceneNode["name"])
        scene->name = sceneNode["name"].as<std::string>();

    // Scene-level properties
    if (sceneNode["shadows_enabled"])
        scene->shadowsEnabled = sceneNode["shadows_enabled"].as<bool>();
    if (sceneNode["pcf_radius"])
        scene->pcfRadius = sceneNode["pcf_radius"].as<int>();
    if (sceneNode["depth_sort_enabled"])
        scene->depthSortEnabled = sceneNode["depth_sort_enabled"].as<bool>();
    if (sceneNode["show_axes"])
        scene->showAxes = sceneNode["show_axes"].as<bool>();
    if (sceneNode["background"]) {
        scene->backgroundType = parseBackgroundType(
            sceneNode["background"].as<std::string>());
        if (scene->backgroundType == BackgroundType::SKYBOX && sceneNode["skybox"]) {
            auto sb = sceneNode["skybox"];
            scene->background = std::make_unique<Skybox>(
                sb["px"].as<std::string>(),
                sb["nx"].as<std::string>(),
                sb["py"].as<std::string>(),
                sb["ny"].as<std::string>(),
                sb["pz"].as<std::string>(),
                sb["nz"].as<std::string>()
            );
        } else if (scene->backgroundType == BackgroundType::HDR_PANORAMA && sceneNode["hdr_panorama"]) {
            auto hdr = sceneNode["hdr_panorama"];
            scene->background = std::make_unique<HdrPanorama>(
                hdr["path"].as<std::string>()
            );
        } else {
            scene->background = std::unique_ptr<Background>(
                BackgroundFactory::createBackground(scene->backgroundType));
        }
    }

    // Camera
    if (sceneNode["camera"])
        parseCamera(sceneNode["camera"], scene->camera);

    // Solids
    if (sceneNode["solids"]) {
        for (const auto& solidNode : sceneNode["solids"]) {
            parseEntity(solidNode, *scene);
        }
    }

    scene->Scene::setup();

    return scene;
}
