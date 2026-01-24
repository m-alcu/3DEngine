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

#include "InputHandler.hpp"
#include "renderer.hpp"
#include "scenes/sceneFactory.hpp"
#include "vendor/imgui/imgui_impl_sdlrenderer3.h"
#include <stdio.h>
#include <algorithm>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

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

  int selectedSolidIndex = 0;

  // Main loop
  bool closedWindow = false;
  std::map<int, bool> keys;
  InputHandler inputHandler(window, keys);
#ifdef __EMSCRIPTEN__
  // For an Emscripten build we are disabling file-system access, so let's not
  // attempt to do a fopen() of the imgui.ini file. You may manually call
  // LoadIniSettingsFromMemory() to load settings from your own storage.
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!keys[SDLK_ESCAPE] && !closedWindow)
#endif
  {
    // Process SDL events
    closedWindow = inputHandler.processEvents(scene, selectedSolidIndex);

    // Process keyboard input for camera movement (Descent-style 6DOF)
    scene->processKeyboardInput(keys);

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
              scene->solids[selectedSolidIndex]->getWorldCenter();
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
        selectedSolidIndex = 0;
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

    solidRenderer.drawScene(*scene);

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
