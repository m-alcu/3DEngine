// Dear ImGui: standalone example application for SDL3 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows,
// inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/
// folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL3.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and
// SDL+OpenGL on Linux/OSX.

#include "projection.hpp"
#include "renderer.hpp"
#include "scene.hpp"
#include "scenes/sceneFactory.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_sdl3.h"
#include "vendor/imgui/imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

namespace {

// Minimal vertex struct for projection
struct PickVertex {
  slib::vec4 ndc;
  int32_t p_x = 0;
  int32_t p_y = 0;
  float p_z = 0;
  bool broken = false;
};

slib::vec3 getSolidWorldCenter(const Solid &solid) {
  slib::vec3 localCenter{(solid.minCoord.x + solid.maxCoord.x) * 0.5f,
                         (solid.minCoord.y + solid.maxCoord.y) * 0.5f,
                         (solid.minCoord.z + solid.maxCoord.z) * 0.5f};
  slib::mat4 rotate = smath::rotation(
      slib::vec3({solid.position.xAngle, solid.position.yAngle,
                  solid.position.zAngle}));
  slib::mat4 translate = smath::translation(
      slib::vec3({solid.position.x, solid.position.y, solid.position.z}));
  slib::mat4 scale = smath::scale(slib::vec3(
      {solid.position.zoom, solid.position.zoom, solid.position.zoom}));
  slib::mat4 modelMatrix = translate * rotate * scale;
  slib::vec4 world = modelMatrix * slib::vec4(localCenter, 1.0f);
  return {world.x, world.y, world.z};
}

} // namespace

// Main code
int main(int, char **) {
  // Setup SDL
  // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts
  // would likely be your SDL_AppInit() function]
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    printf("Error: SDL_Init(): %s\n", SDL_GetError());
    return -1;
  }

  int width = 640;
  int height = 480;

  Renderer solidRenderer;

  // Create window with SDL_Renderer graphics context
  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  SDL_Window *window =
      SDL_CreateWindow("3D Engine", width * 2, height * 2, window_flags);
  if (window == nullptr) {
    printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return -1;
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  SDL_SetRenderVSync(renderer, 1);
  if (renderer == nullptr) {
    SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
    return -1;
  }

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, width, height);
  if (texture == nullptr) {
    SDL_Log("Error: SDL_CreateTexture(): %s\n", SDL_GetError());
    return -1;
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can
  // also load multiple fonts and use ImGui::PushFont()/PopFont() to select
  // them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
  // need to select the font among multiple.
  // - If the file cannot be loaded, the function will return a nullptr. Please
  // handle those errors in your application (e.g. use an assertion, or display
  // an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored
  // into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which
  // ImGui_ImplXXXX_NewFrame below will call.
  // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype
  // for higher quality font rendering.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string
  // literal you need to write a double backslash \\ !
  // - Our Emscripten build process allows embedding fonts to be accessible at
  // runtime from the "fonts/" folder. See Makefile.emscripten for details.
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // nullptr, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);

  auto scene =
      SceneFactory::createScene(SceneType::SHADOWTEST, {height, width});
  scene->setup();

  float lastMouseX = 0, lastMouseY = 0;
  int selectedSolidIndex = 0;

  // After scene.setup();
    if (!scene->solids.empty()) {
      selectedSolidIndex = 0;
      scene->camera.orbitTarget = getSolidWorldCenter(*scene->solids[0]);
    }
    scene->camera.setOrbitFromCurrent();

  // Main loop
  bool closedWindow = false;
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not
  // attempt to do a fopen() of the imgui.ini file. You may manually call
  // LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  for (std::map<int, bool> keys; !keys[SDLK_ESCAPE] && !closedWindow;)
#endif
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application, or clear/overwrite your copy of the
    // keyboard data. Generally you may always pass all inputs to dear imgui,
    // and hide them from your application based on those two flags. [If using
    // SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your
    // SDL_AppEvent() function] Process events.
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
          closedWindow = true;
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (ev.button.button == SDL_BUTTON_RIGHT) {
          // Respect ImGui focus: do not orbit if ImGui wants the mouse
          if (!ImGui::GetIO().WantCaptureMouse) {
            scene->orbiting = true;
            SDL_GetMouseState(&lastMouseX, &lastMouseY);
            SDL_SetWindowRelativeMouseMode(
                window, true); // hide cursor + get relative deltas
          }
        }
        if (ev.button.button == SDL_BUTTON_LEFT) {
          if (!ImGui::GetIO().WantCaptureMouse && !scene->solids.empty()) {
            int windowW = 0;
            int windowH = 0;
            SDL_GetWindowSizeInPixels(window, &windowW, &windowH);
            if (windowW > 0 && windowH > 0) {
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
              Projection<PickVertex> projection;
              for (size_t i = 0; i < scene->solids.size(); ++i) {
                slib::vec3 worldCenter =
                    getSolidWorldCenter(*scene->solids[i]);
                PickVertex pv;
                pv.ndc = slib::vec4(worldCenter, 1.0f) * scene->spaceMatrix;
                if (pv.ndc.w <= 0.0001f) {
                  continue;
                }
                projection.view(scene->screen.width, scene->screen.height, pv,
                                true);
                int64_t dx = pv.p_x - mouseXFP;
                int64_t dy = pv.p_y - mouseYFP;
                int64_t dist2 = dx * dx + dy * dy;
                if (dist2 < bestDist2) {
                  bestDist2 = dist2;
                  bestIndex = static_cast<int>(i);
                }
              }
              if (bestIndex >= 0) {
                selectedSolidIndex = bestIndex;
                scene->camera.orbitTarget =
                    getSolidWorldCenter(*scene->solids[bestIndex]);
                scene->camera.setOrbitFromCurrent();
              }
            }
          }
        }
        break;
      case SDL_EVENT_MOUSE_BUTTON_UP:
        if (ev.button.button == SDL_BUTTON_RIGHT && scene->orbiting) {
          scene->orbiting = false;
          SDL_SetWindowRelativeMouseMode(window, false);
          /*
          slib::vec3 eye = scene.camera.pos;
          slib::vec3 target = scene.camera.orbitTarget;

          // 1) Forward (camera looks from eye -> target)
          slib::vec3 forward = smath::normalize(target - eye);

          // 2) Yaw & pitch (radians), matching your zaxis convention:
          // zaxis = { sinYaw*cosPitch, -sinPitch, cosYaw*cosPitch }
            ne.camera.yaw = std::atan2(forward.x, -forward.z); // [-pi, pi]
            scene.camera.pitch = std::asin(-forward.y);            // [-pi/2,
          pi/2]
          */
        }
        break;
      case SDL_EVENT_MOUSE_WHEEL:
        // Zoom (wheel.y > 0 => zoom in)
        if (!ImGui::GetIO().WantCaptureMouse) {
          float zoomStep = 0.9f; // multiplicative for nicer feel
          if (ev.wheel.y > 0)
            scene->camera.orbitRadius *= zoomStep;
          if (ev.wheel.y < 0)
            scene->camera.orbitRadius /= zoomStep;
          scene->camera.orbitRadius = std::max(0.1f, scene->camera.orbitRadius);
          scene->camera.applyOrbit();
        }
        break;
      case SDL_EVENT_MOUSE_MOTION:
        if (scene->orbiting) {
          // Relative mode gives motion.xrel/yrel directly
          float dx = static_cast<float>(ev.motion.xrel);
          float dy = static_cast<float>(ev.motion.yrel);

          // Tune these two if orbit feels too slow/fast
          const float orbitYawSpeed = 0.0035f; // radians per pixel
          const float orbitPitchSpeed = 0.0035f;

          scene->camera.orbitAzimuth -=
              dx * orbitYawSpeed; // drag right => orbit right
          scene->camera.orbitElevation -=
              dy * orbitPitchSpeed; // drag up   => orbit up
          // Clamp elevation in apply
          scene->camera.applyOrbit();
        }
        break;
      }
    }

    // The input scheme is the same as in Descent, the game by Parallax
    // Interactive.
    bool up = keys[SDLK_UP] || keys[SDLK_KP_8];
    bool down = keys[SDLK_DOWN] || keys[SDLK_KP_2],
         alt = keys[SDLK_LALT] || keys[SDLK_RALT];
    bool left = keys[SDLK_LEFT] || keys[SDLK_KP_4],
         rleft = keys[SDLK_Q] || keys[SDLK_KP_7];
    bool right = keys[SDLK_RIGHT] || keys[SDLK_KP_6],
         rright = keys[SDLK_E] || keys[SDLK_KP_9];
    bool fwd = keys[SDLK_A], sup = keys[SDLK_KP_MINUS], sleft = keys[SDLK_KP_1];
    bool back = keys[SDLK_Z], sdown = keys[SDLK_KP_PLUS],
         sright = keys[SDLK_KP_3];

    // Calculate input deltas
    float yawInput = scene->camera.sensitivity * (right - left);
    float pitchInput = scene->camera.sensitivity * (up - down);
    float rollInput = scene->camera.sensitivity * (rleft - rright);
    float moveInput = (fwd - back) * scene->camera.speed;

    if (!scene->orbiting) { // No free-fly when orbiting

      // Update camera using momentum
      // Apply hysteresis to rotation momentum
      // Change the rotation momentum vector (r) with hysteresis: newvalue =
      // oldvalue*(1-eagerness) + input*eagerness
      scene->rotationMomentum.x =
          scene->rotationMomentum.x * (1.0f - scene->camera.eagerness) +
          pitchInput * scene->camera.eagerness;
      scene->rotationMomentum.y =
          scene->rotationMomentum.y * (1.0f - scene->camera.eagerness) +
          yawInput * scene->camera.eagerness;
      scene->rotationMomentum.z =
          scene->rotationMomentum.z * (1.0f - scene->camera.eagerness) +
          rollInput * scene->camera.eagerness;

      scene->camera.pitch -= scene->rotationMomentum.x;
      scene->camera.yaw -= scene->rotationMomentum.y;
      scene->camera.roll += scene->rotationMomentum.z;
      scene->camera.pos += scene->movementMomentum;

      float pitch = scene->camera.pitch;
      float yaw = scene->camera.yaw;
      float cosPitch = cos(pitch);
      float sinPitch = sin(pitch);
      float cosYaw = cos(yaw);
      float sinYaw = sin(yaw);
      slib::vec3 zaxis = {sinYaw * cosPitch, -sinPitch, -cosPitch * cosYaw};
      scene->camera.forward = zaxis;

      // Apply hysteresis to movement momentum
      scene->movementMomentum =
          scene->movementMomentum * (1.0f - scene->camera.eagerness) +
          scene->camera.forward * moveInput * scene->camera.eagerness;

    } else {
      scene->camera.forward =
          smath::normalize(scene->camera.orbitTarget - scene->camera.pos);
    }

    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your
    // SDL_AppIterate() function]
    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    // Start the Dear ImGui frame
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowBgAlpha(0.3f);

    static float incXangle = 0.0f;
    static float incYangle = 0.0f;
    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    {
      ImGui::Begin("3d params");
      ImGui::SliderFloat("rot x angle", &incXangle, 0.0f, 1.0f);
      ImGui::SliderFloat("rot y angle", &incYangle, 0.0f, 1.0f);
      ImGui::SliderFloat("cam speed", &scene->camera.speed, 0.1f, 10.0f);
      ImGui::SliderFloat("pitch/yaw/roll sens", &scene->camera.sensitivity,
                         0.0f, 10.0f);

      if (!scene->solids.empty()) {
        selectedSolidIndex =
            std::clamp(selectedSolidIndex, 0,
                       static_cast<int>(scene->solids.size() - 1));
        std::vector<std::string> solidLabels;
        solidLabels.reserve(scene->solids.size());
        std::vector<const char *> solidLabelPtrs;
        solidLabelPtrs.reserve(scene->solids.size());
        for (size_t i = 0; i < scene->solids.size(); ++i) {
          const std::string &solidName = scene->solids[i]->name;
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
          scene->camera.orbitTarget =
              getSolidWorldCenter(*scene->solids[selectedSolidIndex]);
          scene->camera.setOrbitFromCurrent();
        }

        Solid *selectedSolid = scene->solids[selectedSolidIndex].get();

        int currentShading = static_cast<int>(selectedSolid->shading);
        if (ImGui::Combo("Shading", &currentShading, shadingNames,
                         IM_ARRAYSIZE(shadingNames))) {
          selectedSolid->shading = static_cast<Shading>(currentShading);
        }

        ImGui::Checkbox("Rotate", &selectedSolid->rotationEnabled);
        float position[3] = {selectedSolid->position.x,
                             selectedSolid->position.y,
                             selectedSolid->position.z};
        if (ImGui::DragFloat3("Position", position, 1.0f)) {
          selectedSolid->position.x = position[0];
          selectedSolid->position.y = position[1];
          selectedSolid->position.z = position[2];
        }

        ImGui::DragFloat("Zoom", &selectedSolid->position.zoom, 0.1f, 0.01f,
                         500.0f);

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
          selectedSolid->orbit_.center =
              {orbitCenter[0], orbitCenter[1], orbitCenter[2]};
        }

        ImGui::DragFloat("Orbit Radius", &selectedSolid->orbit_.radius, 0.1f,
                         0.0f, 10000.0f);
        ImGui::DragFloat("Orbit Speed", &selectedSolid->orbit_.omega, 0.01f,
                         -10.0f, 10.0f);
      }

      int currentBackground = static_cast<int>(scene->backgroundType);
      if (ImGui::Combo("Background", &currentBackground, backgroundNames,
                       IM_ARRAYSIZE(backgroundNames))) {
        // Update the enum value when selection changes
        scene->backgroundType = static_cast<BackgroundType>(currentBackground);
        scene->background = std::unique_ptr<Background>(
            BackgroundFactory::createBackground(scene->backgroundType));
      }

      int currentScene = static_cast<int>(scene->sceneType);
      if (ImGui::Combo("Scene", &currentScene, sceneNames,
                       IM_ARRAYSIZE(sceneNames))) {
        // unique_pointer does manage memory, no need to delete
        scene = SceneFactory::createScene(static_cast<SceneType>(currentScene),
                                          {height, width});
        scene->setup();
        // Update orbitTarget to first solid's position
        if (!scene->solids.empty()) {
          selectedSolidIndex = 0;
          scene->camera.orbitTarget = getSolidWorldCenter(*scene->solids[0]);
        }
        scene->camera.setOrbitFromCurrent();
        scene->backgroundType = static_cast<BackgroundType>(currentBackground);
        scene->background = std::unique_ptr<Background>(
            BackgroundFactory::createBackground(scene->backgroundType));
      }

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / io.Framerate, io.Framerate);
      ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", scene->camera.pos.x,
                  scene->camera.pos.y, scene->camera.pos.z);
      ImGui::Text("Camera for: (%.2f, %.2f, %.2f)", scene->camera.forward.x,
                  scene->camera.forward.y, scene->camera.forward.z);
      ImGui::Text("OrbitTarget: (%.2f, %.2f, %.2f)",
                  scene->camera.orbitTarget.x, scene->camera.orbitTarget.y,
                  scene->camera.orbitTarget.z);
      ImGui::Text("Camera Pitch: %.2f, Yaw: %.2f, Roll: %.2f",
                  scene->camera.pitch, scene->camera.yaw, scene->camera.roll);

      ImGui::Checkbox("Show Shadow Map Overlay", &scene->showShadowMapOverlay);

      // PCF Radius control (0 = no filtering, 1 = 3x3, 2 = 5x5)
      static const char* pcfLabels[] = { "Off (0)", "3x3 (1)", "5x5 (2)" };
      int currentPcfRadius = scene->pcfRadius;
      if (ImGui::Combo("PCF Radius", &currentPcfRadius, pcfLabels, IM_ARRAYSIZE(pcfLabels))) {
        scene->pcfRadius = currentPcfRadius;
        scene->pcfRadiusChanged.InvokeAllCallbacks();
      }

      ImGui::End();
    }

    scene->update(io.DeltaTime);

    solidRenderer.drawScene(*scene, scene->zNear, scene->zFar,
                            scene->viewAngle);

    if (scene->showShadowMapOverlay) {
      solidRenderer.drawShadowMapOverlay(*scene);
    }

    // Rendering
    ImGui::Render();

    SDL_UpdateTexture(texture, nullptr, &scene->pixels[0], 4 * width);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);

    for (auto &solidPtr : scene->solids) {
      // Update the solid's position based on the input angles
      if (solidPtr->rotationEnabled) {
        // Rotate the solid around its local axes
        solidPtr->rotate(incXangle, incYangle, 0.0f);
      }

      solidPtr->updateOrbit(io.DeltaTime);
    }
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  // Cleanup
  // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your
  // SDL_AppQuit() function]
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
  // Destroy the SDL_Texture, SDL_Renderer, and SDL_Window
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
