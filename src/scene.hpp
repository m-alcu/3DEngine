#pragma once
#include <algorithm> // for std::fill
#include <cmath>
#include <cstdint>   // for uint32_t
#include <limits>
#include <map>
#include <memory>    // for std::unique_ptr
#include <ranges>
#include <string>
#include <vector>
#include <SDL3/SDL_keycode.h>

#include "shadow_map.hpp"
#include "z_buffer.hpp"
#include "backgrounds/background.hpp"
#include "backgrounds/background_factory.hpp"
#include "cubemap.hpp"
#include "camera.hpp"
#include "fonts.hpp"
#include "light.hpp"
#include "ecs/registry.hpp"
#include "ecs/name_component.hpp"
#include "ecs/transform_system.hpp"
#include "ecs/light_system.hpp"
#include "ecs/rotation_system.hpp"
#include "ecs/mesh_system.hpp"
#include "ecs/shadow_system.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "stats.hpp"

enum class SceneType {
  YAML,
  BUILTIN_COUNT = YAML
};

typedef struct Screen {
  int32_t width;
  int32_t height;
} Screen;

class Scene {
public:
  // Constructor that initializes the Screen and allocates zBuffer arrays.
  Scene(const Screen &scr)
      : screen(scr), zBuffer(std::make_shared<ZBuffer>(scr.width, scr.height)),
        spaceMatrix(smath::identity()) {
    pixels = new uint32_t[screen.width * screen.height];
    backg = new uint32_t[screen.width * screen.height];
  }

  // Destructor to free the allocated memory.
  ~Scene() {
    delete[] pixels;
    delete[] backg;
  }

  // Called to set up the Scene, including creation of entities, etc.
  // Derived classes should call Scene::setup() at the end of their setup.
  virtual void setup() {
    // Initialize camera orbit target to first entity
    if (!entities.empty()) {
      Entity entity = entities[0];
      auto* transform = registry.transforms().get(entity);
      auto* mesh = registry.meshes().get(entity);
      if (transform && mesh) {
        TransformSystem::updateTransform(*transform);
        camera.orbitTarget = TransformSystem::getWorldCenter(*transform);
      }
    }
    MeshSystem::updateAllBoundsIfDirty(registry.meshes());
    camera.setOrbitFromCurrent();
  }

  virtual void update(float dt) {

    // --- System pipeline: iterate component stores directly ---
    TransformSystem::updateAllOrbits(registry.transforms(), dt);
    RotationSystem::updateAll(registry);
    TransformSystem::updateAllTransforms(registry.transforms());
    LightSystem::syncPositions(registry);
    ShadowSystem::ensureShadowMaps(registry.shadows(), pcfRadius);
    MeshSystem::updateAllBoundsIfDirty(registry.meshes());

    // --- Scene center & radius from entity positions + mesh radius ---
    slib::vec3 sum{};
    int count = 0;
    for (auto& [entity, t] : registry.transforms()) {
      if (!registry.lights().has(entity)) {
        sum += slib::vec3{t.position.x, t.position.y, t.position.z};
        count++;
      }
    }
    if (count > 0) {
      sceneCenter = sum * (1.0f / count);
      float maxDist = 0.0f;
      for (auto& [entity, t] : registry.transforms()) {
        if (!registry.lights().has(entity)) {
          slib::vec3 d{t.position.x - sceneCenter.x,
                       t.position.y - sceneCenter.y,
                       t.position.z - sceneCenter.z};
          float dist = std::sqrt(smath::dot(d, d));
          auto* mesh = registry.meshes().get(entity);
          float r = mesh ? mesh->radius * t.position.zoom : 0.0f;
          if (dist + r > maxDist) maxDist = dist + r;
        }
      }
      sceneRadius = std::max(maxDist, 1.0f);
    } else {
      sceneCenter = {0.0f, 0.0f, -400.0f};
      sceneRadius = 125.0f;
    }
  }

  Entity createEntity() {
    Entity entity = registry.createEntity();
    entities.push_back(entity);
    return entity;
  }

  CubeMap* getCubeMap() const { return background ? background->getCubeMap() : nullptr; }

  slib::vec3 getWorldCenter(Entity entity) const {
    auto* transform = registry.transforms().get(entity);
    if (!transform) {
      return {0.0f, 0.0f, 0.0f};
    }
    return TransformSystem::getWorldCenter(*transform);
  }

  void drawBackground() const {
    float aspectRatio = static_cast<float>(screen.width) / screen.height;
    background->draw(backg, screen.height, screen.width, camera, aspectRatio);
  }

  void clearAllEntities() {
    entities.clear();
    registry.clear();
  }

  Screen screen;
  SceneType sceneType = SceneType::YAML;
  std::string name;

  slib::vec3 forwardNeg; // Negative forward vector for lighting calculations
  slib::mat4 spaceMatrix;
  std::shared_ptr<ZBuffer> zBuffer; // Use shared_ptr for zBuffer to manage its
                                    // lifetime automatically.
  uint32_t *pixels = nullptr;           // Pointer to the pixel data.

  Camera camera; // Camera object to manage camera properties.
  std::vector<Entity> entities;
  Registry registry; // ECS registry for component storage

  // Returns a list of entities that are light sources
  std::vector<Entity> lightSourceEntities() const {
    std::vector<Entity> result;
    result.reserve(registry.lights().size());
    for (const auto& [entity, light] : registry.lights()) {
      result.push_back(entity);
    }
    return result;
  }

  // Returns a list of entities that are renderables (no light component)
  std::vector<Entity> renderableEntities() const {
    std::vector<Entity> result;
    result.reserve(registry.renders().size());
    for (const auto& [entity, render] : registry.renders()) {
      if (!registry.transforms().has(entity) || !registry.meshes().has(entity) ||
          !registry.materials().has(entity)) {
        continue;
      }
      result.push_back(entity);
    }
    return result;
  }

  // Access light components through registry (for effect pixel shaders)
  auto& lights() { return registry.lights(); }
  const auto& lights() const { return registry.lights(); }

  auto& shadows() { return registry.shadows(); }
  const auto& shadows() const { return registry.shadows(); }

  bool orbiting = false;
  bool shadowsEnabled = true;        // Enable/disable shadow rendering
  bool showShadowMapOverlay = false; // Show/hide shadow map debug overlay
  bool showAxes = false;             // Show/hide axis helper
  Stats stats;                       // Rendering statistics  
  bool depthSortEnabled = true;      // Enable/disable face depth sorting
  Font8x8::FontType font = Font8x8::FontType::ZXSpectrum;

  slib::vec3 sceneCenter{};
  float sceneRadius = 0.0f;

  BackgroundType backgroundType = BackgroundType::DESERT;
  uint32_t *backg = nullptr;
  std::unique_ptr<Background> background = std::unique_ptr<Background>(
      BackgroundFactory::createBackground(backgroundType));

  // PCF radius control (0 = no filtering, 1 = 3x3, 2 = 5x5)
  int pcfRadius = SHADOW_PCF_RADIUS;

  // Selected entity index for UI
  int selectedEntityIndex = 0;

};
