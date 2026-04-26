#include "application.hpp"

#include "scene_ui.hpp"
#include "scenes/scene_factory.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_sdl3.h"
#include "vendor/imgui/imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>
#include <cstdio>
#include <memory>

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
  if (!window.create(config.windowTitle,
                     config.screen.width * config.windowScale,
                     config.screen.height * config.windowScale,
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
                      SDL_TEXTUREACCESS_STREAMING, config.screen.width,
                      config.screen.height)) {
    SDL_Log("Error: SDL_CreateTexture(): %s\n", SDL_GetError());
    return false;
  }
  SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_NONE);
  SDL_SetWindowPosition(window.get(), SDL_WINDOWPOS_CENTERED,
                        SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window.get());

  imgui.init(window.get(), sdlRenderer.get());

  state.scene = SceneFactory::createSceneByIndex(state.currentSceneIndex, config.screen);
  state.scene->setup();
  inputHandler = std::make_unique<InputHandler>(window.get(), state.keys);

  return true;
}

int Application::run() {
  ImGuiIO& io = ImGui::GetIO();

#ifdef __EMSCRIPTEN__
  io.IniFilename = nullptr;
  EMSCRIPTEN_MAINLOOP_BEGIN
#else
  while (!state.keys[SDLK_ESCAPE] && !state.closedWindow)
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
  state.closedWindow = inputHandler->processEvents(state.scene);
  inputHandler->processKeyboardInput(state.scene);
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
  state.scene->update(io.DeltaTime);
}

void Application::renderScene() {
  solidRenderer.drawScene(*state.scene);
}

void Application::presentFrame() {
  ImGui::Render();

  SDL_UpdateTexture(texture.get(), nullptr, state.scene->pixels.data(), 4 * config.screen.width);
  SDL_RenderTexture(sdlRenderer.get(), texture.get(), nullptr, nullptr);

  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(),
                                        sdlRenderer.get());
  SDL_RenderPresent(sdlRenderer.get());
}

void Application::drawUi() {
  ImGuiIO& io = ImGui::GetIO();

  ImGui::Begin("3d params");
  SceneUI::drawCameraControls(*state.scene);
  SceneUI::drawSolidControls(*state.scene);
  SceneUI::drawSceneSelector(state, config.screen);
  SceneUI::drawSceneControls(*state.scene);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / io.Framerate, io.Framerate);
  SceneUI::drawCameraInfo(*state.scene);
  SceneUI::drawStats(*state.scene);

  ImGui::End();
}
