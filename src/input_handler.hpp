#pragma once

#include "projection.hpp"
#include "scene.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <map>
#include <memory>

namespace {

// Minimal vertex struct for projection/picking
struct PickVertex {
  slib::vec4 ndc;
  int32_t p_x = 0;
  int32_t p_y = 0;
  float p_z = 0;
  bool broken = false;
};

} // namespace

class InputHandler {
public:
  InputHandler(SDL_Window* win, std::map<int, bool>& keyMap)
      : window(win), keys(keyMap) {}

  // Process keyboard input for camera movement (Descent-style 6DOF)
  void processKeyboardInput(std::unique_ptr<Scene>& scene) {
    auto pressed = [&](int key) { auto it = keys.find(key); return it != keys.end() && it->second; };

    bool up    = pressed(SDLK_UP)    || pressed(SDLK_KP_8);
    bool down  = pressed(SDLK_DOWN)  || pressed(SDLK_KP_2);
    bool left  = pressed(SDLK_LEFT)  || pressed(SDLK_KP_4);
    bool right = pressed(SDLK_RIGHT) || pressed(SDLK_KP_6);
    bool rleft = pressed(SDLK_Q)     || pressed(SDLK_KP_7);
    bool rright= pressed(SDLK_E)     || pressed(SDLK_KP_9);
    bool fwd   = pressed(SDLK_A);
    bool back  = pressed(SDLK_Z);

    float yawInput = scene->camera.sensitivity * (left - right);
    float pitchInput = scene->camera.sensitivity * (up - down);
    float rollInput = scene->camera.sensitivity * (rleft - rright);
    float moveInput = (fwd - back) * scene->camera.speed;

    if (!scene->orbiting) {
      rotationMomentum.x =
          rotationMomentum.x * (1.0f - scene->camera.eagerness) +
          pitchInput * scene->camera.eagerness;
      rotationMomentum.y =
          rotationMomentum.y * (1.0f - scene->camera.eagerness) +
          yawInput * scene->camera.eagerness;
      rotationMomentum.z =
          rotationMomentum.z * (1.0f - scene->camera.eagerness) +
          rollInput * scene->camera.eagerness;

      scene->camera.pitch -= rotationMomentum.x;
      scene->camera.yaw -= rotationMomentum.y;
      scene->camera.roll += rotationMomentum.z;
      scene->camera.pos += movementMomentum;

      float cosPitch = std::cos(scene->camera.pitch);
      float sinPitch = std::sin(scene->camera.pitch);
      float cosYaw = std::cos(scene->camera.yaw);
      float sinYaw = std::sin(scene->camera.yaw);
      slib::vec3 zaxis = {sinYaw * cosPitch, -sinPitch, -cosPitch * cosYaw};
      scene->camera.forward = zaxis;

      movementMomentum =
          movementMomentum * (1.0f - scene->camera.eagerness) +
          scene->camera.forward * moveInput * scene->camera.eagerness;
    } else {
      scene->camera.forward = smath::normalize(scene->camera.orbitTarget - scene->camera.pos);
    }
  }

  // Process all pending SDL events
  // Returns true if window close was requested
  bool processEvents(std::unique_ptr<Scene>& scene) {
    for (SDL_Event ev; SDL_PollEvent(&ev);) {
      ImGui_ImplSDL3_ProcessEvent(&ev);

      switch (ev.type) {
      case SDL_EVENT_QUIT:
        keys[SDLK_ESCAPE] = true;
        break;
      case SDL_EVENT_KEY_DOWN:
        keys[ev.key.key] = true;
        break;
      case SDL_EVENT_KEY_UP:
        keys[ev.key.key] = false;
        break;
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (ev.window.windowID == SDL_GetWindowID(window)) {
          return true; // Window close requested
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        handleMouseButtonDown(ev, scene);
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        handleMouseButtonUp(ev, scene);
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        handleMouseWheel(ev, scene);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        handleMouseMotion(ev, scene);
        break;
      }
    }
    return false;
  }

private:
  SDL_Window* window;
  std::map<int, bool>& keys;
  float lastMouseX = 0;
  float lastMouseY = 0;
  slib::vec3 rotationMomentum{};
  slib::vec3 movementMomentum{};

  void handleMouseButtonDown(const SDL_Event& ev,
                             std::unique_ptr<Scene>& scene) {
    if (ev.button.button == SDL_BUTTON_RIGHT) {
      // Respect ImGui focus: do not orbit if ImGui wants the mouse
      if (!ImGui::GetIO().WantCaptureMouse) {
        scene->orbiting = true;
        SDL_GetMouseState(&lastMouseX, &lastMouseY);
        SDL_SetWindowRelativeMouseMode(window, true);
      }
    }

    if (ev.button.button == SDL_BUTTON_LEFT) {
      if (!ImGui::GetIO().WantCaptureMouse && !scene->entities.empty()) {
        pickSolid(ev, scene);
      }
    }
  }

  void pickSolid(const SDL_Event& ev, std::unique_ptr<Scene>& scene) {
    int windowW = 0;
    int windowH = 0;
    SDL_GetWindowSizeInPixels(window, &windowW, &windowH);

    if (windowW <= 0 || windowH <= 0) {
      return;
    }

    // Convert mouse to 16.16 fixed-point in scene coordinates
    constexpr int32_t FP = 65536; // 1<<16
    int32_t mouseXFP = static_cast<int32_t>(
        (ev.button.x * scene->screen.width / windowW + 0.5f) * FP);
    int32_t mouseYFP = static_cast<int32_t>(
        (ev.button.y * scene->screen.height / windowH + 0.5f) * FP);

    // Pick radius in 16.16 fixed-point (28 pixels)
    constexpr int64_t pickRadiusFP = 28 * FP;
    int64_t bestDist2 = pickRadiusFP * pickRadiusFP;
    int bestIndex = -1;

    for (size_t i = 0; i < scene->entities.size(); ++i) {
      slib::vec3 worldCenter = scene->getWorldCenter(scene->entities[i]);
      PickVertex pv;
      pv.ndc = slib::vec4(worldCenter, 1.0f) * scene->spaceMatrix;

      if (!Projection<PickVertex>::view(scene->screen.width,
                                        scene->screen.height, pv, true)) {
        continue;
      }

      int64_t dx = pv.p_x - mouseXFP;
      int64_t dy = pv.p_y - mouseYFP;
      int64_t dist2 = dx * dx + dy * dy;

      if (dist2 < bestDist2) {
        bestDist2 = dist2;
        bestIndex = static_cast<int>(i);
      }
    }

    if (bestIndex >= 0) {
      scene->selectedEntityIndex = bestIndex;
      scene->camera.orbitTarget =
          scene->getWorldCenter(scene->entities[bestIndex]);
      scene->camera.setOrbitFromCurrent();
    }
  }

  void handleMouseButtonUp(const SDL_Event& ev,
                           std::unique_ptr<Scene>& scene) {
    if (ev.button.button == SDL_BUTTON_RIGHT && scene->orbiting) {
      scene->orbiting = false;
      SDL_SetWindowRelativeMouseMode(window, false);
    }
  }

  void handleMouseWheel(const SDL_Event& ev, std::unique_ptr<Scene>& scene) {
    if (!ImGui::GetIO().WantCaptureMouse) {
      float zoomStep = 0.9f;
      if (ev.wheel.y > 0)
        scene->camera.orbitRadius *= zoomStep;
      if (ev.wheel.y < 0)
        scene->camera.orbitRadius /= zoomStep;
      scene->camera.orbitRadius = std::max(0.1f, scene->camera.orbitRadius);
      scene->camera.applyOrbit();
    }
  }

  void handleMouseMotion(const SDL_Event& ev, std::unique_ptr<Scene>& scene) {
    if (scene->orbiting) {
      float dx = static_cast<float>(ev.motion.xrel);
      float dy = static_cast<float>(ev.motion.yrel);

      const float orbitYawSpeed = 0.0035f;
      const float orbitPitchSpeed = 0.0035f;

      scene->camera.orbitAzimuth -= dx * orbitYawSpeed;
      scene->camera.orbitElevation -= dy * orbitPitchSpeed;
      scene->camera.applyOrbit();
    }
  }
};
