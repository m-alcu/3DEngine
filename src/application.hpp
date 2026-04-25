#pragma once

#include "input_handler.hpp"
#include "renderer.hpp"
#include "scene.hpp"

#include <SDL3/SDL.h>
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
  void shutdown();

  SDL_Window* window = nullptr;
  SDL_Renderer* sdlRenderer = nullptr;
  SDL_Texture* texture = nullptr;

  Renderer solidRenderer;
  std::unique_ptr<Scene> scene;
  std::unique_ptr<InputHandler> inputHandler;
  std::map<int, bool> keys;

  bool closedWindow = false;
  bool initialized = false;
  int currentSceneIndex = 0;
};
