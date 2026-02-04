#include "sceneFactory.hpp"
#include "amigaScene.hpp"
#include "bunnyScene.hpp"
#include "cubeScene.hpp"
#include "icosahedronScene.hpp"
#include "knotScene.hpp"
#include "knotScenePoint.hpp"
#include "shadowPointTestScene.hpp"
#include "shadowTestScene.hpp"
#include "starScene.hpp"
#include "suzanneScene.hpp"
#include "tetrakisScene.hpp"
#include "vikingRoomScene.hpp"
#include "isometricLevelScene.hpp"
#include "mountainsScene.hpp"
#include "catStatueScene.hpp"
#include "sponzaScene.hpp"
#include "torusScene.hpp"
#include "worldScene.hpp"

std::unique_ptr<Scene> SceneFactory::createScene(SceneType type, Screen scr) {
  switch (type) {
  case SceneType::AMIGA:
    return std::make_unique<AmigaScene>(scr);
  case SceneType::CUBE:
    return std::make_unique<CubeScene>(scr);
  case SceneType::ICOSAHEDRON:
    return std::make_unique<IcosahedronScene>(scr);
  case SceneType::KNOT:
    return std::make_unique<KnotScene>(scr);
  case SceneType::KNOT_POINT:
    return std::make_unique<KnotScenePoint>(scr);
  case SceneType::BUNNY:
    return std::make_unique<BunnyScene>(scr);
  case SceneType::SUZANNE:
    return std::make_unique<SuzanneScene>(scr);
  case SceneType::VIKING_ROOM:
    return std::make_unique<VikingRoomScene>(scr);
  case SceneType::ISOMETRIC_LEVEL:
    return std::make_unique<IsometricLevelScene>(scr);
  case SceneType::MOUNTAINS:
    return std::make_unique<MountainsScene>(scr);
  case SceneType::CAT_STATUE:
    return std::make_unique<CatStatueScene>(scr);
  case SceneType::STAR:
    return std::make_unique<StarScene>(scr);
  case SceneType::TETRAKIS:
    return std::make_unique<TetrakisScene>(scr);
  case SceneType::TORUS:
    return std::make_unique<TorusScene>(scr);
  case SceneType::WORLD:
    return std::make_unique<WorldScene>(scr);
  case SceneType::SHADOWTEST:
    return std::make_unique<ShadowTestScene>(scr);
  case SceneType::SHADOWTEST_POINT:
    return std::make_unique<ShadowPointTestScene>(scr);
  case SceneType::SPONZA:
    return std::make_unique<SponzaScene>(scr);
  default:
    return nullptr;
  }
}
