#pragma once
#include <algorithm> // for std::fill
#include <cmath>
#include <cstdint>   // for uint32_t
#include <limits>
#include <map>
#include <memory>    // for std::unique_ptr
#include <string>
#include <vector>
#include <SDL3/SDL_keycode.h>

#include "ShadowMap.hpp"
#include "vendor/imgui/imgui.h"
#include "ZBuffer.hpp"
#include "backgrounds/background.hpp"
#include "backgrounds/backgroundFactory.hpp"
#include "camera.hpp"
#include "events/Event.hpp"
#include "light.hpp"
#include "objects/solid.hpp"
#include "slib.hpp"
#include "smath.hpp"

enum class SceneType {
  TORUS,
  TETRAKIS,
  ICOSAHEDRON,
  CUBE,
  KNOT,
  KNOT_POINT,
  BUNNY,
  STAR,
  AMIGA,
  WORLD,
  SHADOWTEST,
  SHADOWTEST_POINT
};

static const char *sceneNames[] = {
    "Torus",      "Tetrakis",    "Icosahedron",       "Cube",  "Knot",
    "Knot Point", "Bunny",       "Star",              "Amiga", "World",
    "Shadow Test", "Shadow Point Test"};

typedef struct Screen {
  int32_t width;
  int32_t height;
} Screen;

class Scene {
public:
  // Constructor that initializes the Screen and allocates zBuffer arrays.
  Scene(const Screen &scr)
      : screen(scr), zBuffer(std::make_shared<ZBuffer>(scr.width, scr.height)),
        shadowMap(std::make_shared<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE)),
        spaceMatrix(smath::identity()) {
    pixels = new uint32_t[screen.width * screen.height];
    backg = new uint32_t[screen.width * screen.height];
    // Subscribe shadow map to pcfRadius changes
    shadowMap->subscribeToPcfRadiusChanges(pcfRadiusChanged, pcfRadius);
  }

  // Destructor to free the allocated memory.
  ~Scene() {
    delete[] pixels;
    delete[] backg;
  }

  // Called to set up the Scene, including creation of Solids, etc.
  // Derived classes should call Scene::setup() at the end of their setup.
  virtual void setup() {
    // Initialize camera orbit target to first solid
    if (!solids.empty()) {
      solids[selectedSolidIndex]->calculateTransformMat();
      camera.orbitTarget = solids[selectedSolidIndex]->getWorldCenter();
    }
    camera.setOrbitFromCurrent();
  }

  virtual void update(float dt) {

    bool hasGeometry = false;
    worldBoundMin = {std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max()};
    worldBoundMax = {-std::numeric_limits<float>::max(),
                     -std::numeric_limits<float>::max(),
                     -std::numeric_limits<float>::max()};

    // Update all solids (rotation, orbit, transform matrix)
    for (auto &solidPtr : solids) {
      if (solidPtr->rotationEnabled) {
        solidPtr->rotate(solidPtr->incXangle, solidPtr->incYangle, 0.0f);
      }
      solidPtr->updateOrbit(dt);
      solidPtr->calculateTransformMat();
      hasGeometry = true;
    }

    // Sync light from first light-source solid, or use defaultLight
    // Also ensure each light-source solid has a shadow map
    bool foundLightSource = false;
    for (auto &solidPtr : solids) {
      if (solidPtr->lightSourceEnabled) {
        // Create shadow map for this light source if it doesn't exist
        if (!solidPtr->shadowMap) {
          solidPtr->shadowMap = std::make_shared<ShadowMap>(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
          solidPtr->shadowMap->subscribeToPcfRadiusChanges(pcfRadiusChanged, pcfRadius);
        }

        // Update light position from solid position
        solidPtr->light.position = {solidPtr->position.x, solidPtr->position.y, solidPtr->position.z};

        // Use first light source as active light
        if (!foundLightSource) {
          light = solidPtr->light;
          foundLightSource = true;
        }
      }
    }
    usingDefaultLight = !foundLightSource;
    if (usingDefaultLight) {
      light = defaultLight;
    }

    // Calculate world bounds

    for (auto &solidPtr : solids) {
      if (solidPtr->lightSourceEnabled) {
        continue; // Skip light sources
      }
      solidPtr->updateWorldBounds(worldBoundMin, worldBoundMax);
    }

    // Calculate scene center and radius
    if (hasGeometry) {
      sceneCenter = {(worldBoundMin.x + worldBoundMax.x) * 0.5f,
                     (worldBoundMin.y + worldBoundMax.y) * 0.5f,
                     (worldBoundMin.z + worldBoundMax.z) * 0.5f};
      slib::vec3 diag{worldBoundMax.x - worldBoundMin.x,
                      worldBoundMax.y - worldBoundMin.y,
                      worldBoundMax.z - worldBoundMin.z};
      float diagLen2 = smath::dot(diag, diag);
      sceneRadius = 0.5f * std::sqrt(diagLen2);
      sceneRadius = std::max(sceneRadius, 1.0f);
    } else {
      sceneCenter = {0.0f, 0.0f, -400.0f};
      sceneRadius = 125.0f;
    }
  }

  // Process keyboard input for camera movement (Descent-style 6DOF)
  void processKeyboardInput(const std::map<int, bool>& keys) {
    bool up = keys.count(SDLK_UP) && keys.at(SDLK_UP);
    up = up || (keys.count(SDLK_KP_8) && keys.at(SDLK_KP_8));
    bool down = keys.count(SDLK_DOWN) && keys.at(SDLK_DOWN);
    down = down || (keys.count(SDLK_KP_2) && keys.at(SDLK_KP_2));
    bool left = keys.count(SDLK_LEFT) && keys.at(SDLK_LEFT);
    left = left || (keys.count(SDLK_KP_4) && keys.at(SDLK_KP_4));
    bool right = keys.count(SDLK_RIGHT) && keys.at(SDLK_RIGHT);
    right = right || (keys.count(SDLK_KP_6) && keys.at(SDLK_KP_6));
    bool rleft = keys.count(SDLK_Q) && keys.at(SDLK_Q);
    rleft = rleft || (keys.count(SDLK_KP_7) && keys.at(SDLK_KP_7));
    bool rright = keys.count(SDLK_E) && keys.at(SDLK_E);
    rright = rright || (keys.count(SDLK_KP_9) && keys.at(SDLK_KP_9));
    bool fwd = keys.count(SDLK_A) && keys.at(SDLK_A);
    bool back = keys.count(SDLK_Z) && keys.at(SDLK_Z);

    // Calculate input deltas
    float yawInput = camera.sensitivity * (right - left);
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

  // Add a solid to the scene's list of solids.
  // Using std::unique_ptr is a good practice for ownership.
  void addSolid(std::unique_ptr<Solid> solid) {
    solids.push_back(std::move(solid));
  }

  void drawBackground() const {

    background->draw(backg, screen.height, screen.width);
  }

  // Add a solid to the scene's list of solids.
  // Using std::unique_ptr is a good practice for ownership.
  void clearAllSolids() { solids.clear(); }

  Screen screen;
  SceneType sceneType = SceneType::TETRAKIS; // Default scene type

  Light light;           // Active light (synced from light-source solid or defaultLight)
  Light defaultLight;     // Fallback light when no solid has lightSourceEnabled
  bool usingDefaultLight = true;  // True if using defaultLight (no light-source solid)
  slib::vec3 forwardNeg; // Negative forward vector for lighting calculations
  slib::mat4 spaceMatrix;
  std::shared_ptr<ZBuffer> zBuffer; // Use shared_ptr for zBuffer to manage its
                                    // lifetime automatically.
  std::shared_ptr<ShadowMap> shadowMap; // Shadow map for scene lighting
  uint32_t *pixels = nullptr;           // Pointer to the pixel data.

  slib::vec3 rotationMomentum{0.f, 0.f,
                              0.f}; // Rotation momentum vector (nonzero
                                    // indicates view is still rotating)
  slib::vec3 movementMomentum{0.f, 0.f,
                              0.f}; // Movement momentum vector (nonzero
                                    // indicates camera is still moving)

  Camera camera; // Camera object to manage camera properties.
  // Store solids in a vector of unique_ptr to handle memory automatically.
  std::vector<std::unique_ptr<Solid>> solids;
  bool orbiting = false;
  bool shadowsEnabled = true;        // Enable/disable shadow rendering
  bool showShadowMapOverlay = false; // Show/hide shadow map debug overlay
  bool showAxes = false;             // Show/hide axis helper

  // World bounds calculated in update()
  slib::vec3 worldBoundMin{};
  slib::vec3 worldBoundMax{};
  slib::vec3 sceneCenter{};
  float sceneRadius = 0.0f;

  BackgroundType backgroundType = BackgroundType::DESERT;
  uint32_t *backg = nullptr;
  std::unique_ptr<Background> background = std::unique_ptr<Background>(
      BackgroundFactory::createBackground(backgroundType));

  // PCF radius control (0 = no filtering, 1 = 3x3, 2 = 5x5)
  int pcfRadius = SHADOW_PCF_RADIUS;
  sage::Event pcfRadiusChanged;

  // Selected solid index for UI
  int selectedSolidIndex = 0;

  // Draw ImGui controls for solid editing
  void drawSolidControls() {
    if (solids.empty()) return;

    selectedSolidIndex = std::clamp(selectedSolidIndex, 0,
                                    static_cast<int>(solids.size() - 1));

    // Build solid labels for combo
    std::vector<std::string> solidLabels;
    solidLabels.reserve(solids.size());
    std::vector<const char*> solidLabelPtrs;
    solidLabelPtrs.reserve(solids.size());

    for (size_t i = 0; i < solids.size(); ++i) {
      const std::string& solidName = solids[i]->name;
      if (!solidName.empty()) {
        solidLabels.push_back(solidName);
      } else {
        solidLabels.push_back("Solid " + std::to_string(i));
      }
      solidLabelPtrs.push_back(solidLabels.back().c_str());
    }

    if (ImGui::Combo("Selected Solid", &selectedSolidIndex,
                     solidLabelPtrs.data(),
                     static_cast<int>(solidLabelPtrs.size()))) {
      camera.orbitTarget = solids[selectedSolidIndex]->getWorldCenter();
      camera.setOrbitFromCurrent();
    }

    Solid* selectedSolid = solids[selectedSolidIndex].get();

    int currentShading = static_cast<int>(selectedSolid->shading);
    if (ImGui::Combo("Shading", &currentShading, shadingNames,
                     IM_ARRAYSIZE(shadingNames))) {
      selectedSolid->shading = static_cast<Shading>(currentShading);
    }

    ImGui::Checkbox("Rotate", &selectedSolid->rotationEnabled);
    ImGui::SliderFloat("Rot X Speed", &selectedSolid->incXangle, 0.0f, 1.0f);
    ImGui::SliderFloat("Rot Y Speed", &selectedSolid->incYangle, 0.0f, 1.0f);

    float position[3] = {selectedSolid->position.x,
                         selectedSolid->position.y,
                         selectedSolid->position.z};
    if (ImGui::DragFloat3("Position", position, 1.0f)) {
      selectedSolid->position.x = position[0];
      selectedSolid->position.y = position[1];
      selectedSolid->position.z = position[2];
    }

    ImGui::DragFloat("Zoom", &selectedSolid->position.zoom, 0.1f, 0.01f, 500.0f);

    float angles[3] = {selectedSolid->position.xAngle,
                       selectedSolid->position.yAngle,
                       selectedSolid->position.zAngle};
    if (ImGui::DragFloat3("Angles", angles, 1.0f, -360.0f, 360.0f)) {
      selectedSolid->position.xAngle = angles[0];
      selectedSolid->position.yAngle = angles[1];
      selectedSolid->position.zAngle = angles[2];
    }

    bool orbitEnabled = selectedSolid->orbit_.enabled;
    if (ImGui::Checkbox("Enable Orbit", &orbitEnabled)) {
      if (orbitEnabled) {
        selectedSolid->enableCircularOrbit(selectedSolid->orbit_.center,
                                           selectedSolid->orbit_.radius,
                                           selectedSolid->orbit_.n,
                                           selectedSolid->orbit_.omega,
                                           selectedSolid->orbit_.phase);
      } else {
        selectedSolid->disableCircularOrbit();
      }
    }

    float orbitCenter[3] = {selectedSolid->orbit_.center.x,
                            selectedSolid->orbit_.center.y,
                            selectedSolid->orbit_.center.z};
    if (ImGui::DragFloat3("Orbit Center", orbitCenter, 1.0f)) {
      selectedSolid->orbit_.center = {orbitCenter[0], orbitCenter[1], orbitCenter[2]};
    }

    ImGui::DragFloat("Orbit Radius", &selectedSolid->orbit_.radius, 0.1f, 0.0f, 10000.0f);
    ImGui::DragFloat("Orbit Speed", &selectedSolid->orbit_.omega, 0.01f, -10.0f, 10.0f);
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
    ImGui::Checkbox("Show Shadow Map Overlay", &showShadowMapOverlay);

    static const char* pcfLabels[] = {"Off (0)", "3x3 (1)", "5x5 (2)"};
    int currentPcfRadius = pcfRadius;
    if (ImGui::Combo("PCF Radius", &currentPcfRadius, pcfLabels, IM_ARRAYSIZE(pcfLabels))) {
      pcfRadius = currentPcfRadius;
      pcfRadiusChanged.InvokeAllCallbacks();
    }
  }

  // Draw camera info text
  void drawCameraInfo() {
    ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", camera.pos.x, camera.pos.y, camera.pos.z);
    ImGui::Text("Camera for: (%.2f, %.2f, %.2f)", camera.forward.x, camera.forward.y, camera.forward.z);
    ImGui::Text("OrbitTarget: (%.2f, %.2f, %.2f)", camera.orbitTarget.x, camera.orbitTarget.y, camera.orbitTarget.z);
    ImGui::Text("Camera Pitch: %.2f, Yaw: %.2f, Roll: %.2f", camera.pitch, camera.yaw, camera.roll);
  }
};
