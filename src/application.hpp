#pragma once

#include "app_config.hpp"
#include "app_state.hpp"
#include "input_handler.hpp"
#include "platform_resources.hpp"
#include "renderer.hpp"

#include <memory>

class Application {
public:
  Application() = default;
  ~Application();

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  bool init();
  int run();

private:
  void processInput();
  bool shouldPauseFrame() const;
  void beginUiFrame();
  void drawUi();
  void updateScene();
  void renderScene();
  void presentFrame();
  void runFrame();

  SdlContext sdl;
  SdlWindow window;
  SdlRenderer sdlRenderer;
  SdlTexture texture;
  ImguiContext imgui;

  Renderer solidRenderer;
  std::unique_ptr<InputHandler> inputHandler;

  AppConfig config;
  AppState state;
};
