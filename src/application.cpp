#include "application.hpp"

#include "backgrounds/background_factory.hpp"
#include "constants.hpp"
#include "scene_ui.hpp"
#include "scenes/scene_factory.hpp"
#include <cstdio>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

Application::~Application() = default;

bool Application::init() {
  if (!sdl.init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    std::printf("Error: SDL_Init(): %s\n", SDL_GetError());
    return false;
  }

  SDL_WindowFlags windowFlags =
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
  if (!window.create("3D Engine", SCREEN_WIDTH * 2, SCREEN_HEIGHT * 2,
                     windowFlags)) {
    std::printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    return false;
  }

  if (!sdlRenderer.create(window.get())) {
    SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
    return false;
  }
  SDL_SetRenderVSync(sdlRenderer.get(), 1);

  if (!texture.create(sdlRenderer.get(), SDL_PIXELFORMAT_ARGB8888,
                      SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
                      SCREEN_HEIGHT)) {
    SDL_Log("Error: SDL_CreateTexture(): %s\n", SDL_GetError());
    return false;
  }
  SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE);
  SDL_SetWindowPosition(window.get(), SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window.get());

  imgui.init(window.get(), sdlRenderer.get());

  scene = SceneFactory::createSceneByIndex(currentSceneIndex,
                                           {SCREEN_WIDTH, SCREEN_HEIGHT});
  scene->setup();
  inputHandler = std::make_unique<InputHandler>(window.get(), keys);

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
  processInput();

  if (shouldPauseFrame()) {
    SDL_Delay(10);
    return;
  }

  beginUiFrame();
  drawUi();
  updateScene();
  renderScene();
  presentFrame();
}

void Application::processInput() {
  closedWindow = inputHandler->processEvents(scene);
  inputHandler->processKeyboardInput(scene);
}

bool Application::shouldPauseFrame() const {
  return SDL_GetWindowFlags(window.get()) & SDL_WINDOW_MINIMIZED;
}

void Application::beginUiFrame() {
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  ImGui::SetNextWindowBgAlpha(0.3f);
}

void Application::updateScene() {
  ImGuiIO& io = ImGui::GetIO();
  scene->update(io.DeltaTime);
}

void Application::renderScene() {
  solidRenderer.drawScene(*scene);
}

void Application::presentFrame() {
  ImGui::Render();

  SDL_UpdateTexture(texture.get(), nullptr, &scene->pixels[0], 4 * SCREEN_WIDTH);
  SDL_RenderTexture(sdlRenderer.get(), texture.get(), nullptr, nullptr);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),
                                        sdlRenderer.get());
  SDL_RenderPresent(sdlRenderer.get());
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
