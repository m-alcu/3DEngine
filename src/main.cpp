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
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", width * 2, height * 2, window_flags);
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
    scene.lux = smath::normalize(slib::vec3{0, 1, 1});;
    scene.eye = {0, 0, 1};
    scene.camera.pos = {0, 0, 0};
    scene.camera.pitch = 0;
    scene.camera.yaw = 0;
    scene.setup();

    float zNear = 10.0f; // Near plane distance
    float zFar  = 10000.0f; // Far plane distance
    float viewAngle = 45.0f; // Field of view angle in degrees    

    BackgroundType backgroundType = BackgroundType::DESERT; // Default background type

    // Backgroud
    Uint32* back = new Uint32[width * height];
    auto background = std::unique_ptr<Background>(BackgroundFactory::createBackground(backgroundType));
    background->draw(back, height, width);

    static float mouseSensitivity = 1.0f;
    static float cameraSpeed = 6.0f;

    // Main loop
    bool done = false;
#ifdef __EMSCRIPTEN__
    // For an Emscripten build we are disabling file-system access, so let's not attempt to do a fopen() of the imgui.ini file.
    // You may manually call LoadIniSettingsFromMemory() to load settings from your own storage.
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!done)
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_KEY_DOWN) {
                SDL_KeyboardEvent* key_event = (SDL_KeyboardEvent*)&event;
                SDL_Keycode keycode = key_event->key;

                switch (keycode) {
                    case SDLK_LEFT:
                        scene.camera.yaw += mouseSensitivity;
                        break;
                    case SDLK_RIGHT:
                        scene.camera.yaw -= mouseSensitivity;
                        break;
                    case SDLK_UP:
                        scene.camera.pitch -= mouseSensitivity;
                        break;
                    case SDLK_DOWN:
                        scene.camera.pitch += mouseSensitivity;
                        break;
                    case SDLK_Q:
                        scene.camera.pos -= scene.camera.forward * cameraSpeed;
                        break;
                    case SDLK_A:
                        scene.camera.pos += scene.camera.forward * cameraSpeed;
                        break;
                }
            }


            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

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
            ImGui::SliderFloat("x angle", &incXangle, 0.0f, 1.0f); 
            ImGui::SliderFloat("y angle", &incYangle, 0.0f, 1.0f); 
            ImGui::SliderFloat("camera speed", &cameraSpeed, 0.1f, 10.0f); 
            ImGui::SliderFloat("pitch/yaw sensitivity", &mouseSensitivity, 0.0f, 10.0f);

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

        background->draw(back, height, width);
        solidRenderer.drawScene(scene, zNear, zFar, viewAngle, back);

        // Rendering
        ImGui::Render();

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, scene.sdlSurface);
        if (!tex) {
            SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
        }

        SDL_RenderTexture(renderer, tex, nullptr, nullptr);

        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);

        SDL_DestroyTexture(tex);

        scene.solids[0]->position.xAngle += incXangle;
        scene.solids[0]->position.yAngle += incYangle;
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    // Cleanup
    // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    delete[] back;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
