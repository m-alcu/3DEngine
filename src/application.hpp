#pragma once

#include "input_handler.hpp"
#include "platform_resources.hpp"
#include "renderer.hpp"
#include "scene.hpp"

#include <map>
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
  std::unique_ptr<Scene> scene;
  std::unique_ptr<InputHandler> inputHandler;
  std::map<int, bool> keys;

  bool closedWindow = false;
  int currentSceneIndex = 0;
};
