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

  Renderer solidRenderer;

  // Create window with SDL_Renderer graphics context
  SDL_WindowFlags window_flags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  SDL_Window *window =
      SDL_CreateWindow("3D Engine", SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2, window_flags);
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
                        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
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
      SceneFactory::createScene(SceneType::SHADOWTEST, {SCREEN_WIDTH, SCREEN_HEIGHT});
  scene->setup();

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
    closedWindow = inputHandler.processEvents(scene);

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

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair
    // to create a named window.
    {
      ImGui::Begin("3d params");
      ImGui::SliderFloat("cam speed", &scene->camera.speed, 0.1f, 10.0f);
      ImGui::SliderFloat("pitch/yaw/roll sens", &scene->camera.sensitivity,
                         0.0f, 10.0f);

      scene->drawSolidControls();

      int currentBackground = static_cast<int>(scene->backgroundType);
      int currentScene = static_cast<int>(scene->sceneType);
      if (ImGui::Combo("Scene", &currentScene, sceneNames,
                       IM_ARRAYSIZE(sceneNames))) {
        scene = SceneFactory::createScene(static_cast<SceneType>(currentScene),
                                          {SCREEN_WIDTH, SCREEN_HEIGHT});
        scene->setup();
        scene->backgroundType = static_cast<BackgroundType>(currentBackground);
        scene->background = std::unique_ptr<Background>(
            BackgroundFactory::createBackground(scene->backgroundType));
      }

      scene->drawSceneControls();

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / io.Framerate, io.Framerate);
      scene->drawCameraInfo();

      ImGui::End();
    }

    scene->update(io.DeltaTime);

    solidRenderer.drawScene(*scene, io.DeltaTime);

    // Rendering
    ImGui::Render();

    SDL_UpdateTexture(texture, nullptr, &scene->pixels[0], 4 * SCREEN_WIDTH);
    SDL_RenderTexture(renderer, texture, nullptr, nullptr);

    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);

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
