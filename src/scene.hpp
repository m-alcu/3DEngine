#pragma once
#include <algorithm> // for std::fill
#include <cstdint>   // for uint32_t
#include <memory>    // for std::unique_ptr
#include <vector>

#include "ShadowMap.hpp"
#include "ZBuffer.hpp"
#include "backgrounds/background.hpp"
#include "backgrounds/backgroundFactory.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "objects/amiga.hpp"
#include "objects/ascLoader.hpp"
#include "objects/cube.hpp"
#include "objects/icosahedron.hpp"
#include "objects/objLoader.hpp"
#include "objects/solid.hpp"
#include "objects/test.hpp"
#include "objects/tetrakis.hpp"
#include "objects/torus.hpp"
#include "objects/world.hpp"
#include "slib.hpp"
#include "smath.hpp"

enum class SceneType {
  TORUS,
  TETRAKIS,
  ICOSAHEDRON,
  CUBE,
  KNOT,
  KNOT_POINT,
  STAR,
  AMIGA,
  WORLD,
  SHADOWTEST,
  SHADOWTEST_POINT
};

static const char *sceneNames[] = {
    "Torus",      "Tetrakis",    "Icosahedron",       "Cube",  "Knot",
    "Knot Point", "Star",        "Amiga",             "World", "Shadow Test",
    "Shadow Point Test"};

typedef struct Screen {
  int32_t height;
  int32_t width;
} Screen;

class Scene {
public:
  // Constructor that initializes the Screen and allocates zBuffer arrays.
  Scene(const Screen &scr)
      : screen(scr), zBuffer(std::make_shared<ZBuffer>(scr.width, scr.height)),
        shadowMap(std::make_shared<ShadowMap>(512, 512)),
        spaceMatrix(smath::identity()) {
    pixels = new uint32_t[screen.width * screen.height];
    backg = new uint32_t[screen.width * screen.height];
  }

  // Destructor to free the allocated memory.
  ~Scene() {
    delete[] pixels;
    delete[] backg;
  }

  // Called to set up the Scene, including creation of Solids, etc.
  virtual void setup() {
    // Default implementation does nothing.
    // Derived classes should override this method.
  }

  virtual void update(float dt) {
    // Default implementation does nothing.
  }

  // Add a solid to the scene's list of solids.
  // Using std::unique_ptr is a good practice for ownership.
  void addSolid(std::unique_ptr<Solid> solid) {
    solids.push_back(std::move(solid));
  }

  void drawBackground() const {

    background->draw(backg, screen.height, screen.width);
  }

  // Add a solid to the scene's list of solids.
  // Using std::unique_ptr is a good practice for ownership.
  void clearAllSolids() { solids.clear(); }

  Screen screen;
  SceneType sceneType = SceneType::TETRAKIS; // Default scene type

  Light light;
  slib::vec3 forwardNeg; // Negative forward vector for lighting calculations
  slib::mat4 spaceMatrix;
  std::shared_ptr<ZBuffer> zBuffer; // Use shared_ptr for zBuffer to manage its
                                    // lifetime automatically.
  std::shared_ptr<ShadowMap> shadowMap; // Shadow map for shadow rendering
  uint32_t *pixels = nullptr;           // Pointer to the pixel data.

  slib::vec3 rotationMomentum{0.f, 0.f,
                              0.f}; // Rotation momentum vector (nonzero
                                    // indicates view is still rotating)
  slib::vec3 movementMomentum{0.f, 0.f,
                              0.f}; // Movement momentum vector (nonzero
                                    // indicates camera is still moving)

  float zNear;     // Near plane distance
  float zFar;      // Far plane distance
  float viewAngle; // Field of view angle in degrees

  Camera camera; // Camera object to manage camera properties.
  // Store solids in a vector of unique_ptr to handle memory automatically.
  std::vector<std::unique_ptr<Solid>> solids;
  bool orbiting = false;
  bool shadowsEnabled = true;        // Enable/disable shadow rendering
  bool showShadowMapOverlay = false; // Show/hide shadow map debug overlay

  BackgroundType backgroundType = BackgroundType::DESERT;
  uint32_t *backg = nullptr;
  std::unique_ptr<Background> background = std::unique_ptr<Background>(
      BackgroundFactory::createBackground(backgroundType));
};
