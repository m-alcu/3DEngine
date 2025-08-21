// Dear ImGui: standalone example application for SDL3 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL3.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_sdl3.h"
#include "vendor/imgui/imgui_impl_sdlrenderer3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#include "backgrounds/background.hpp"
#include "backgrounds/backgroundFactory.hpp"
#include "renderer.hpp"
#include "scene.hpp"

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

// Main code
int main(int, char**)
{
    // Setup SDL
    // [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    int width = 640;
    int height = 480;

    Renderer solidRenderer;

    // Create window with SDL_Renderer graphics context
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("3D Engine", width * 2, height * 2, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == nullptr)
    {
        SDL_Log("Error: SDL_CreateTexture(): %s\n", SDL_GetError());
        return -1;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    Scene scene({height, width});
    scene.setup();

    BackgroundType backgroundType = BackgroundType::DESERT; // Default background type

    // Backgroud
    Uint32* backg = new Uint32[width * height];
    auto background = std::unique_ptr<Background>(BackgroundFactory::createBackground(backgroundType));
    background->draw(backg, height, width);

    // Main loop
    bool closedWindow = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    for (std::map<int, bool> keys; !keys[SDLK_ESCAPE] && !closedWindow; )
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        // Process events.
        for (SDL_Event ev; SDL_PollEvent(&ev); ) {

            ImGui_ImplSDL3_ProcessEvent(&ev);

            switch (ev.type)
            {
                case SDL_EVENT_QUIT: keys[SDLK_ESCAPE] = true; break;
                case SDL_EVENT_KEY_DOWN: keys[ev.key.key] = true; break;
                case SDL_EVENT_KEY_UP:   keys[ev.key.key] = false; break;
				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					if (ev.window.windowID == SDL_GetWindowID(window)) {
						closedWindow = true;
					}
					break;
            }
        }

        // The input scheme is the same as in Descent, the game by Parallax Interactive.
        // Mouse input is not handled for now.
        bool up = keys[SDLK_UP] || keys[SDLK_KP_8];
        bool down = keys[SDLK_DOWN] || keys[SDLK_KP_2], alt = keys[SDLK_LALT] || keys[SDLK_RALT];
        bool left = keys[SDLK_LEFT] || keys[SDLK_KP_4], rleft = keys[SDLK_Q] || keys[SDLK_KP_7];
        bool right = keys[SDLK_RIGHT] || keys[SDLK_KP_6], rright = keys[SDLK_E] || keys[SDLK_KP_9];
        bool fwd = keys[SDLK_A], sup = keys[SDLK_KP_MINUS], sleft = keys[SDLK_KP_1];
        bool back = keys[SDLK_Z], sdown = keys[SDLK_KP_PLUS], sright = keys[SDLK_KP_3];

        // Calculate input deltas
        float yawInput = scene.camera.sensitivity * (right - left);
        float pitchInput = scene.camera.sensitivity * (up - down);
        float rollInput = scene.camera.sensitivity * (rleft - rright);
        float moveInput = (fwd - back) * scene.camera.speed;

        // Apply hysteresis to rotation momentum
        scene.rotationMomentum.x = scene.rotationMomentum.x * (1.0f - scene.camera.eagerness) + pitchInput * scene.camera.eagerness;
        scene.rotationMomentum.y = scene.rotationMomentum.y * (1.0f - scene.camera.eagerness) + yawInput * scene.camera.eagerness;
        scene.rotationMomentum.z = scene.rotationMomentum.z * (1.0f - scene.camera.eagerness) + rollInput * scene.camera.eagerness;

        // Apply hysteresis to movement momentum
        scene.movementMomentum = scene.movementMomentum * (1.0f - scene.camera.eagerness) + scene.camera.forward * moveInput * scene.camera.eagerness;

        // Update camera using momentum
        scene.camera.pitch -= scene.rotationMomentum.x;
        scene.camera.yaw -= scene.rotationMomentum.y;
        scene.camera.roll += scene.rotationMomentum.z;
        scene.camera.pos += scene.movementMomentum;
        // Change the rotation momentum vector (r) with hysteresis: newvalue = oldvalue*(1-eagerness) + input*eagerness

        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowBgAlpha(0.3f);

        static float incXangle = 0.5f;
        static float incYangle = 1.0f;
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            ImGui::Begin("3d params");                         
            ImGui::SliderFloat("rot x angle", &incXangle, 0.0f, 1.0f); 
            ImGui::SliderFloat("rot y angle", &incYangle, 0.0f, 1.0f); 
            ImGui::SliderFloat("cam speed", &scene.camera.speed, 0.1f, 10.0f);
            ImGui::SliderFloat("pitch/yaw/roll sens", &scene.camera.sensitivity, 0.0f, 10.0f);

            // Render combo box in your ImGui window code
            int currentShading = static_cast<int>(scene.solids[0]->shading);
            if (ImGui::Combo("Shading", &currentShading, shadingNames, IM_ARRAYSIZE(shadingNames))) {
                // Update the enum value when selection changes
                scene.solids[0]->shading = static_cast<Shading>(currentShading);
            }
            
            int currentBackground = static_cast<int>(backgroundType);
            if (ImGui::Combo("Background", &currentBackground, backgroundNames, IM_ARRAYSIZE(backgroundNames))) {
                // Update the enum value when selection changes
                backgroundType = static_cast<BackgroundType>(currentBackground);
                background = BackgroundFactory::createBackground(backgroundType);
            } 

            int currentScene = static_cast<int>(scene.sceneType);
            if (ImGui::Combo("Scene", &currentScene, sceneNames, IM_ARRAYSIZE(sceneNames))) {
                // Update the enum value when selection changes
                scene.sceneType = static_cast<SceneType>(currentScene);
                scene.setup();
            }

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        background->draw(backg, height, width);

        solidRenderer.drawScene(scene, scene.zNear, scene.zFar, scene.viewAngle, backg);

        // Rendering
        ImGui::Render();

        SDL_UpdateTexture(texture, nullptr, &scene.pixels[0], 4 * width);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        for (auto& solidPtr : scene.solids) {
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
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    delete[] backg;

    SDL_DestroyRenderer(renderer);
	SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
