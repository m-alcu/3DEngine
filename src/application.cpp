#include "application.hpp"

#include "backgrounds/background_factory.hpp"
#include "constants.hpp"
#include "scene_ui.hpp"
#include "scenes/scene_factory.hpp"
#include "vendor/imgui/imgui_impl_sdlrenderer3.h"

#include <cstdio>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

Application::~Application() { shutdown(); }

bool Application::init() {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    std::printf("Error: SDL_Init(): %s\n", SDL_GetError());
    return false;
  }
  initialized = true;

  SDL_WindowFlags windowFlags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  window = SDL_CreateWindow("3D Engine", SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2,
                            windowFlags);
  if (window == nullptr) {
    std::printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return false;
  }

  sdlRenderer = SDL_CreateRenderer(window, nullptr);
  if (sdlRenderer == nullptr) {
    SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
    return false;
  }
  SDL_SetRenderVSync(sdlRenderer, 1);

  texture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                              SCREEN_HEIGHT);
  if (texture == nullptr) {
    SDL_Log("Error: SDL_CreateTexture(): %s\n", SDL_GetError());
    return false;
  }
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForSDLRenderer(window, sdlRenderer);
  ImGui_ImplSDLRenderer3_Init(sdlRenderer);

  scene = SceneFactory::createSceneByIndex(currentSceneIndex,
                                           {SCREEN_WIDTH, SCREEN_HEIGHT});
  scene->setup();
  inputHandler = std::make_unique<InputHandler>(window, keys);

  return true;
}

int Application::run() {
  ImGuiIO& io = ImGui::GetIO();

#ifdef __EMSCRIPTEN__
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!keys[SDLK_ESCAPE] && !closedWindow)
#endif
  {
    runFrame();
  }
#ifdef __EMSCRIPTEN__
  EMSCRIPTEN_MAINLOOP_END;
#endif

  return 0;
}

void Application::runFrame() {
  closedWindow = inputHandler->processEvents(scene);
  inputHandler->processKeyboardInput(scene);

  if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
    SDL_Delay(10);
    return;
  }

  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGui::SetNextWindowBgAlpha(0.3f);

  drawUi();

  ImGuiIO& io = ImGui::GetIO();
  scene->update(io.DeltaTime);
  solidRenderer.drawScene(*scene);

  ImGui::Render();

  SDL_UpdateTexture(texture, nullptr, &scene->pixels[0], 4 * SCREEN_WIDTH);
  SDL_RenderTexture(sdlRenderer, texture, nullptr, nullptr);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdlRenderer);
  SDL_RenderPresent(sdlRenderer);
}

void Application::drawUi() {
  ImGuiIO& io = ImGui::GetIO();

  ImGui::Begin("3d params");
  ImGui::SliderFloat("cam speed", &scene->camera.speed, 0.1f, 10.0f);
  ImGui::SliderFloat("pitch/yaw/roll sens", &scene->camera.sensitivity, 0.0f,
                     10.0f);

  SceneUI::drawSolidControls(*scene);

  int currentBackground = static_cast<int>(scene->backgroundType);
  const auto& names = SceneFactory::allSceneNames();
  auto itemGetter = [](void* data, int idx) -> const char* {
    auto* v = static_cast<const std::vector<std::string>*>(data);
    return (*v)[idx].c_str();
  };
  if (ImGui::Combo("Scene", &currentSceneIndex, itemGetter,
                   const_cast<void*>(static_cast<const void*>(&names)),
                   SceneFactory::sceneCount())) {
    auto newScene = SceneFactory::createSceneByIndex(
        currentSceneIndex, {SCREEN_WIDTH, SCREEN_HEIGHT});
    if (newScene) {
      scene = std::move(newScene);
      scene->setup();
      scene->backgroundType = static_cast<BackgroundType>(currentBackground);
      scene->background = std::unique_ptr<Background>(
          BackgroundFactory::createBackground(scene->backgroundType));
    }
  }

  SceneUI::drawSceneControls(*scene);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / io.Framerate, io.Framerate);
  SceneUI::drawCameraInfo(*scene);
  SceneUI::drawStats(*scene);

  ImGui::End();
}

void Application::shutdown() {
  if (!initialized) {
    return;
  }

  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }

  if (texture != nullptr) {
    SDL_DestroyTexture(texture);
    texture = nullptr;
  }
  if (sdlRenderer != nullptr) {
    SDL_DestroyRenderer(sdlRenderer);
    sdlRenderer = nullptr;
  }
  if (window != nullptr) {
    SDL_DestroyWindow(window);
    window = nullptr;
  }

  SDL_Quit();
  initialized = false;
}
