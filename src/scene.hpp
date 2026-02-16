#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "cubemap.hpp"
#include "fonts.hpp"
#include "light.hpp"
#include "shadow_map.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "stats.hpp"
#include "z_buffer.hpp"
#include "backgrounds/background.hpp"
#include "backgrounds/background_factory.hpp"
#include "ecs/light_system.hpp"
#include "ecs/mesh_system.hpp"
#include "ecs/name_component.hpp"
#include "ecs/registry.hpp"
#include "ecs/rotation_system.hpp"
#include "ecs/shadow_system.hpp"
#include "ecs/transform_system.hpp"

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
  // --- Construction / Destruction ---

  Scene(const Screen &scr)
      : screen(scr), zBuffer(std::make_shared<ZBuffer>(scr.width, scr.height)),
        spaceMatrix(smath::identity()) {
    pixels = new uint32_t[screen.width * screen.height];
    backg = new uint32_t[screen.width * screen.height];
  }

  ~Scene() {
    delete[] pixels;
    delete[] backg;
  }

  // --- Lifecycle ---

  virtual void setup() {
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
    TransformSystem::updateAllOrbits(registry.transforms(), dt);
    RotationSystem::updateAll(registry);
    TransformSystem::updateAllTransforms(registry.transforms());
    LightSystem::syncPositions(registry);
    ShadowSystem::ensureShadowMaps(registry.shadows(), registry.lights(), pcfRadius, useCubemapShadows, cubeShadowMaxSlopeBias);
    MeshSystem::updateAllBoundsIfDirty(registry.meshes());

    updateSceneBounds();
  }

  // --- Entity management ---

  Entity createEntity() {
    Entity entity = registry.createEntity();
    entities.push_back(entity);
    return entity;
  }

  void clearAllEntities() {
    entities.clear();
    registry.clear();
  }

  // --- Queries ---

  slib::vec3 getWorldCenter(Entity entity) const {
    auto* transform = registry.transforms().get(entity);
    if (!transform) return {};
    return TransformSystem::getWorldCenter(*transform);
  }

  std::vector<Entity> lightSourceEntities() const {
    std::vector<Entity> result;
    result.reserve(registry.lights().size());
    for (const auto& [entity, light] : registry.lights()) {
      result.push_back(entity);
    }
    return result;
  }

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

  auto& lights() { return registry.lights(); }
  const auto& lights() const { return registry.lights(); }
  auto& shadows() { return registry.shadows(); }
  const auto& shadows() const { return registry.shadows(); }
  CubeMap* getCubeMap() const { return background ? background->getCubeMap() : nullptr; }

  void drawBackground() const {
    float aspectRatio = static_cast<float>(screen.width) / screen.height;
    background->draw(backg, screen.height, screen.width, camera, aspectRatio);
  }

  // --- ECS data ---

  std::vector<Entity> entities;
  Registry registry;

  // --- Rendering state ---

  Screen screen;
  slib::mat4 spaceMatrix;
  slib::vec3 forwardNeg;
  std::shared_ptr<ZBuffer> zBuffer;
  uint32_t* pixels = nullptr;
  Stats stats;

  // --- Camera ---

  Camera camera;
  bool orbiting = false;

  // --- Scene settings ---

  SceneType sceneType = SceneType::YAML;
  std::string name;
  slib::vec3 sceneCenter{};
  float sceneRadius = 0.0f;

  bool shadowsEnabled = true;
  bool showShadowMapOverlay = false;
  bool showAxes = false;
  bool depthSortEnabled = true;
  int pcfRadius = SHADOW_PCF_RADIUS;
  bool useCubemapShadows = true; // Enable omnidirectional cubemap shadows for point lights
  Font8x8::FontType font = Font8x8::FontType::ZXSpectrum;

  // Shadow bias configuration
  float minBiasDefault = MIN_BIAS_DEFAULT;
  float maxBiasDefault = MAX_BIAS_DEFAULT;
  float shadowBiasMin = SHADOW_BIAS_MIN;
  float shadowBiasMax = SHADOW_BIAS_MAX;
  float cubeShadowMaxSlopeBias = CUBE_SHADOW_MAX_SLOPE_BIAS;

  // --- Background ---

  BackgroundType backgroundType = BackgroundType::DESERT;
  uint32_t* backg = nullptr;
  std::unique_ptr<Background> background = std::unique_ptr<Background>(
      BackgroundFactory::createBackground(backgroundType));

  // --- UI state ---

  int selectedEntityIndex = 0;

private:
  void updateSceneBounds() {
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
};
