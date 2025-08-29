#pragma once
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
#include "backgrounds/background.hpp"
#include "backgrounds/backgroundFactory.hpp"
#include "light.hpp"


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
    slib::vec3 pos = { 0,0,0 };
    float pitch = 0.0f;
    float yaw = 0.0f;
	float roll = 0.0f; // Roll is not used in FPS view, but can be added if needed.
    slib::vec3 forward = { 0,0,0 };
    float eagerness = 0.1f; // 0 = no response, 1 = instant response
    float sensitivity = 0.05f;
    float speed = 25.0f;

    // Orbit parameters
    slib::vec3 orbitTarget{ 0,0,0 };
    float orbitRadius = 5.0f;
    float orbitAzimuth = 0.0f;     // around Y (left/right)
    float orbitElevation = 0.0f;   // up/down, clamp to (-pi/2, pi/2)
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
          projectionMatrix(smath::identity()),
		  viewMatrix(smath::identity())
    {
        pixels = new uint32_t[screen.width * screen.height];
        backg = new uint32_t[screen.width * screen.height];
    }

    // Destructor to free the allocated memory.
    ~Scene()
    {
        delete[] pixels;
        delete[] backg;
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
    void drawBackground();
    void cameraSetOrbitFromCurrent(Camera& cam);
    void cameraApplyOrbit(Camera& cam);

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
	std::vector<Light> lights;
    slib::vec3 halfwayVector;
	slib::vec3 forwardNeg; // Negative forward vector for lighting calculations
    slib::mat4 projectionMatrix;
	slib::mat4 viewMatrix;
    std::shared_ptr<ZBuffer> zBuffer; // Use shared_ptr for zBuffer to manage its lifetime automatically.
    uint32_t* pixels = nullptr; // Pointer to the pixel data.

    slib::vec3 rotationMomentum{ 0.f, 0.f, 0.f }; // Rotation momentum vector (nonzero indicates view is still rotating)
    slib::vec3 movementMomentum{ 0.f, 0.f, 0.f }; // Movement momentum vector (nonzero indicates camera is still moving)

    float zNear; // Near plane distance
    float zFar; // Far plane distance
    float viewAngle; // Field of view angle in degrees   

    Camera camera; // Camera object to manage camera properties.
    // Store solids in a vector of unique_ptr to handle memory automatically.
    std::vector<std::unique_ptr<Solid>> solids;
    bool orbiting = false;

    BackgroundType backgroundType = BackgroundType::DESERT;
    uint32_t* backg = nullptr;
    std::unique_ptr<Background> background = std::unique_ptr<Background>(BackgroundFactory::createBackground(backgroundType));
};
