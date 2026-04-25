#pragma once

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_sdl3.h"
#include "vendor/imgui/imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>

class SdlContext {
public:
  SdlContext() = default;
  ~SdlContext() { reset(); }

  SdlContext(const SdlContext&) = delete;
  SdlContext& operator=(const SdlContext&) = delete;

  bool init(SDL_InitFlags flags) {
    active = SDL_Init(flags);
    return active;
  }

  void reset() {
    if (active) {
      SDL_Quit();
      active = false;
    }
  }

private:
  bool active = false;
};

class SdlWindow {
public:
  SdlWindow() = default;
  ~SdlWindow() { reset(); }

  SdlWindow(const SdlWindow&) = delete;
  SdlWindow& operator=(const SdlWindow&) = delete;

  bool create(const char* title, int width, int height, SDL_WindowFlags flags) {
    reset();
    window = SDL_CreateWindow(title, width, height, flags);
    return window != nullptr;
  }

  SDL_Window* get() const { return window; }

  void reset() {
    if (window != nullptr) {
      SDL_DestroyWindow(window);
      window = nullptr;
    }
  }

private:
  SDL_Window* window = nullptr;
};

class SdlRenderer {
public:
  SdlRenderer() = default;
  ~SdlRenderer() { reset(); }

  SdlRenderer(const SdlRenderer&) = delete;
  SdlRenderer& operator=(const SdlRenderer&) = delete;

  bool create(SDL_Window* window) {
    reset();
    renderer = SDL_CreateRenderer(window, nullptr);
    return renderer != nullptr;
  }

  SDL_Renderer* get() const { return renderer; }

  void reset() {
    if (renderer != nullptr) {
      SDL_DestroyRenderer(renderer);
      renderer = nullptr;
    }
  }

private:
  SDL_Renderer* renderer = nullptr;
};

class SdlTexture {
public:
  SdlTexture() = default;
  ~SdlTexture() { reset(); }

  SdlTexture(const SdlTexture&) = delete;
  SdlTexture& operator=(const SdlTexture&) = delete;

  bool create(SDL_Renderer* renderer, SDL_PixelFormat format,
              SDL_TextureAccess access, int width, int height) {
    reset();
    texture = SDL_CreateTexture(renderer, format, access, width, height);
    return texture != nullptr;
  }

  SDL_Texture* get() const { return texture; }

  void reset() {
    if (texture != nullptr) {
      SDL_DestroyTexture(texture);
      texture = nullptr;
    }
  }

private:
  SDL_Texture* texture = nullptr;
};

class ImguiContext {
public:
  ImguiContext() = default;
  ~ImguiContext() { reset(); }

  ImguiContext(const ImguiContext&) = delete;
  ImguiContext& operator=(const ImguiContext&) = delete;

  void init(SDL_Window* window, SDL_Renderer* renderer) {
    reset();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    active = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
  }

  void reset() {
    if (active) {
      ImGui_ImplSDLRenderer3_Shutdown();
      ImGui_ImplSDL3_Shutdown();
      ImGui::DestroyContext();
      active = false;
    }
  }

private:
  bool active = false;
};
