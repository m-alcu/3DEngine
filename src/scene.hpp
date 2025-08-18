#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>    // for std::unique_ptr
#include <algorithm> // for std::fill
#include <cstdint>   // for uint32_t

#include "objects/solid.hpp"
#include "objects/tetrakis.hpp"
#include "objects/torus.hpp"
#include "objects/amiga.hpp"
#include "objects/world.hpp"
#include "objects/test.hpp"
#include "objects/cube.hpp"
#include "objects/ascLoader.hpp"
#include "objects/objLoader.hpp"
#include "smath.hpp"
#include "slib.hpp"
#include "ZBuffer.hpp"


enum class SceneType {
    TORUS,
    TETRAKIS,
    CUBE,
    KNOT,
    STAR,
    AMIGA,
    WORLD
};

static const char* sceneNames[] = {
    "Torus",
    "Tetrakis",
    "Cube",
    "Knot",
    "Star",
    "Amiga",
    "World"
};

struct Camera
{
    slib::vec3 pos;
    slib::vec3 rotation;    
    slib::vec3 eye;
    slib::vec3 target;
    slib::vec3 up;
    float pitch;
    float yaw;
	float roll = 0.0f; // Roll is not used in FPS view, but can be added if needed.
    slib::vec3 forward;
};

typedef struct Screen
{
    int32_t height;
    int32_t width;
} Screen;

class Scene
{
public:
    // Constructor that initializes the Screen and allocates zBuffer arrays.
    Scene(const Screen& scr)
        : screen(scr),
          zBuffer( std::make_shared<ZBuffer>( scr.width,scr.height )),
          projectionMatrix(smath::identity())
    {
        sdlSurface = SDL_CreateSurface(screen.width, screen.height, SDL_PIXELFORMAT_BGRA32);
        if (!sdlSurface) {
            SDL_Log("SDL_CreateSurface failed: %s", SDL_GetError());
        }

        SDL_SetSurfaceBlendMode(sdlSurface, SDL_BLENDMODE_NONE);
        camera.eye = {0.0f, 0.0f, 0.0f};          // Camera position
        camera.target = {0.0f, 0.0f, -1.0f};      // Point to look at (in -Z)
        camera.up = {0.0f, 1.0f, 0.0f};           // Up vector (typically +Y)
    }

    // Destructor to free the allocated memory.
    ~Scene()
    {
        SDL_DestroySurface(sdlSurface);
    }

    // Called to set up the Scene, including creation of Solids, etc.
    void setup();
    void torusInit();
    void tetrakisInit();
    void cubeInit();
    void knotInit();
    void starInit();
    void amigaInit();
    void worldInit();

    // Add a solid to the scene's list of solids.
    // Using std::unique_ptr is a good practice for ownership.
    void addSolid(std::unique_ptr<Solid> solid)
    {
        solids.push_back(std::move(solid));
    }

    // Add a solid to the scene's list of solids.
    // Using std::unique_ptr is a good practice for ownership.
    void clearAllSolids()
    {
        solids.clear();
    }

    Screen screen;
    SceneType sceneType = SceneType::TETRAKIS; // Default scene type
   
    slib::vec3 lux;
    slib::vec3 eye;
    slib::vec3 halfwayVector;
    slib::mat4 projectionMatrix;
    std::shared_ptr<ZBuffer> zBuffer; // Use shared_ptr for zBuffer to manage its lifetime automatically.
    SDL_Surface* sdlSurface = nullptr; // SDL surface for rendering.

    Camera camera; // Camera object to manage camera properties.
    // Store solids in a vector of unique_ptr to handle memory automatically.
    std::vector<std::unique_ptr<Solid>> solids;
};
