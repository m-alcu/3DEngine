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
#include "vendor/imgui/imgui.h"
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

  // Process keyboard input for camera movement (Descent-style 6DOF)
  void processKeyboardInput(const std::map<int, bool>& keys) {
    auto pressed = [&](int key) { auto it = keys.find(key); return it != keys.end() && it->second; };

    bool up    = pressed(SDLK_UP)    || pressed(SDLK_KP_8);
    bool down  = pressed(SDLK_DOWN)  || pressed(SDLK_KP_2);
    bool left  = pressed(SDLK_LEFT)  || pressed(SDLK_KP_4);
    bool right = pressed(SDLK_RIGHT) || pressed(SDLK_KP_6);
    bool rleft = pressed(SDLK_Q)     || pressed(SDLK_KP_7);
    bool rright= pressed(SDLK_E)     || pressed(SDLK_KP_9);
    bool fwd   = pressed(SDLK_A);
    bool back  = pressed(SDLK_Z);

    // Calculate input deltas
    float yawInput = camera.sensitivity * (left - right);
    float pitchInput = camera.sensitivity * (up - down);
    float rollInput = camera.sensitivity * (rleft - rright);
    float moveInput = (fwd - back) * camera.speed;

    if (!orbiting) { // No free-fly when orbiting
      // Apply hysteresis to rotation momentum
      rotationMomentum.x =
          rotationMomentum.x * (1.0f - camera.eagerness) +
          pitchInput * camera.eagerness;
      rotationMomentum.y =
          rotationMomentum.y * (1.0f - camera.eagerness) +
          yawInput * camera.eagerness;
      rotationMomentum.z =
          rotationMomentum.z * (1.0f - camera.eagerness) +
          rollInput * camera.eagerness;

      camera.pitch -= rotationMomentum.x;
      camera.yaw -= rotationMomentum.y;
      camera.roll += rotationMomentum.z;
      camera.pos += movementMomentum;

      float pitch = camera.pitch;
      float yaw = camera.yaw;
      float cosPitch = std::cos(pitch);
      float sinPitch = std::sin(pitch);
      float cosYaw = std::cos(yaw);
      float sinYaw = std::sin(yaw);
      slib::vec3 zaxis = {sinYaw * cosPitch, -sinPitch, -cosPitch * cosYaw};
      camera.forward = zaxis;

      // Apply hysteresis to movement momentum
      movementMomentum =
          movementMomentum * (1.0f - camera.eagerness) +
          camera.forward * moveInput * camera.eagerness;
    } else {
      camera.forward = smath::normalize(camera.orbitTarget - camera.pos);
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

  slib::vec3 rotationMomentum{0.f, 0.f,
                              0.f}; // Rotation momentum vector (nonzero
                                    // indicates view is still rotating)
  slib::vec3 movementMomentum{0.f, 0.f,
                              0.f}; // Movement momentum vector (nonzero
                                    // indicates camera is still moving)

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

  // Draw ImGui controls for solid editing
  void drawSolidControls() {
    if (entities.empty()) return;

    selectedEntityIndex = std::clamp(selectedEntityIndex, 0,
                                     static_cast<int>(entities.size() - 1));

    // Build solid labels for combo
    std::vector<std::string> solidLabels;
    solidLabels.reserve(entities.size());
    std::vector<const char*> solidLabelPtrs;
    solidLabelPtrs.reserve(entities.size());

    for (size_t i = 0; i < entities.size(); ++i) {
      Entity entity = entities[i];
      const auto* nameComp = registry.names().get(entity);
      const std::string& solidName = nameComp ? nameComp->name : std::string();
      if (!solidName.empty()) {
        solidLabels.push_back(solidName);
      } else {
        solidLabels.push_back("Solid " + std::to_string(i));
      }
      solidLabelPtrs.push_back(solidLabels.back().c_str());
    }

    if (ImGui::Combo("Selected Solid", &selectedEntityIndex,
                     solidLabelPtrs.data(),
                     static_cast<int>(solidLabelPtrs.size()))) {
      Entity entity = entities[selectedEntityIndex];
      auto* transform = registry.transforms().get(entity);
      auto* mesh = registry.meshes().get(entity);
      if (transform && mesh) {
        camera.orbitTarget = TransformSystem::getWorldCenter(*transform);
        camera.setOrbitFromCurrent();
      }
    }

    Entity entity = entities[selectedEntityIndex];
    auto* render = registry.renders().get(entity);
    auto* transform = registry.transforms().get(entity);
    if (!render || !transform) {
      return;
    }

    int currentShading = static_cast<int>(render->shading);
    if (ImGui::Combo("Shading", &currentShading, shadingNames,
                     IM_ARRAYSIZE(shadingNames))) {
      render->shading = static_cast<Shading>(currentShading);
    }

    auto* rotation = registry.rotations().get(entity);
    if (rotation) {
      ImGui::Checkbox("Rotate", &rotation->enabled);
      ImGui::SliderFloat("Rot X Speed", &rotation->incXangle, 0.0f, 1.0f);
      ImGui::SliderFloat("Rot Y Speed", &rotation->incYangle, 0.0f, 1.0f);
    }

    float position[3] = {transform->position.x,
                         transform->position.y,
                         transform->position.z};
    if (ImGui::DragFloat3("Position", position, 1.0f)) {
      transform->position.x = position[0];
      transform->position.y = position[1];
      transform->position.z = position[2];
    }

    ImGui::DragFloat("Zoom", &transform->position.zoom, 0.1f, 0.01f, 500.0f);

    float angles[3] = {transform->position.xAngle,
                       transform->position.yAngle,
                       transform->position.zAngle};
    if (ImGui::DragFloat3("Angles", angles, 1.0f, -360.0f, 360.0f)) {
      transform->position.xAngle = angles[0];
      transform->position.yAngle = angles[1];
      transform->position.zAngle = angles[2];
    }

    bool orbitEnabled = transform->orbit.enabled;
    if (ImGui::Checkbox("Enable Orbit", &orbitEnabled)) {
      if (orbitEnabled) {
        TransformSystem::enableCircularOrbit(*transform,
                                             transform->orbit.center,
                                             transform->orbit.radius,
                                             transform->orbit.n,
                                             transform->orbit.omega,
                                             transform->orbit.phase);
      } else {
        TransformSystem::disableCircularOrbit(*transform);
      }
    }

    float orbitCenter[3] = {transform->orbit.center.x,
                            transform->orbit.center.y,
                            transform->orbit.center.z};
    if (ImGui::DragFloat3("Orbit Center", orbitCenter, 1.0f)) {
      transform->orbit.center = {orbitCenter[0], orbitCenter[1], orbitCenter[2]};
    }

    ImGui::DragFloat("Orbit Radius", &transform->orbit.radius, 0.1f, 0.0f, 10000.0f);
    ImGui::DragFloat("Orbit Speed", &transform->orbit.omega, 0.01f, -10.0f, 10.0f);

    // Light properties (only shown if solid is a light source)
    auto* lightComponent = registry.lights().get(entity);
    if (lightComponent) {
      ImGui::Separator();
      ImGui::Text("Light Source");
      ImGui::SliderFloat("Light Intensity", &lightComponent->light.intensity, 0.0f, 100.0f);
    }
  }

  // Draw ImGui controls for scene-level settings
  void drawSceneControls() {
    int currentBackground = static_cast<int>(backgroundType);
    if (ImGui::Combo("Background", &currentBackground, backgroundNames,
                     IM_ARRAYSIZE(backgroundNames))) {
      backgroundType = static_cast<BackgroundType>(currentBackground);
      background = std::unique_ptr<Background>(
          BackgroundFactory::createBackground(backgroundType));
    }

    ImGui::Checkbox("Show Axis Helper", &showAxes);
    ImGui::Checkbox("Face Depth Sorting", &depthSortEnabled);
    ImGui::Checkbox("Shadows Enabled", &shadowsEnabled);
    ImGui::Checkbox("Show Shadow Map Overlay", &showShadowMapOverlay);

    static const char* pcfLabels[] = {"Off (0)", "3x3 (1)", "5x5 (2)"};
    int currentPcfRadius = pcfRadius;
    if (ImGui::Combo("PCF Radius", &currentPcfRadius, pcfLabels, IM_ARRAYSIZE(pcfLabels))) {
      pcfRadius = currentPcfRadius;
    }

    static const char* fontLabels[] = {"Default", "IBM CGA", "ZX Spectrum", "Amstrad CPC", "Commodore 64", "Atari 8-bit"};
    int currentFont = static_cast<int>(font);
    if (ImGui::Combo("Font", &currentFont, fontLabels, IM_ARRAYSIZE(fontLabels))) {
      font = static_cast<Font8x8::FontType>(currentFont);
    }
  }

  // Draw camera info text
  void drawCameraInfo() {
    ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", camera.pos.x, camera.pos.y, camera.pos.z);
    ImGui::Text("Camera for: (%.2f, %.2f, %.2f)", camera.forward.x, camera.forward.y, camera.forward.z);
    ImGui::Text("OrbitTarget: (%.2f, %.2f, %.2f)", camera.orbitTarget.x, camera.orbitTarget.y, camera.orbitTarget.z);
    ImGui::Text("Camera Pitch: %.2f, Yaw: %.2f, Roll: %.2f", camera.pitch, camera.yaw, camera.roll);
  }

  // Draw rendering stats
  void drawStats() {
    ImGui::Separator();
    ImGui::Text("Polys rendered: %u", stats.polysRendered);
    ImGui::Text("Pixels rasterized: %u", stats.pixelsRasterized);
    ImGui::Text("Draw calls: %u", stats.drawCalls);
    ImGui::Text("Vertices processed: %u", stats.verticesProcessed);
  }

  
};
