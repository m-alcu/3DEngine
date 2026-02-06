#include "sceneFactory.hpp"
#include "sceneLoader.hpp"
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

#include <algorithm>
#include <filesystem>

// Static member definitions
std::vector<std::string> SceneFactory::yamlPaths_;
std::vector<std::string> SceneFactory::yamlNames_;
std::vector<std::string> SceneFactory::combinedNames_;
bool SceneFactory::scanned_ = false;

std::unique_ptr<Scene> SceneFactory::createScene(SceneType type, Screen scr) {
  switch (type) {
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

std::unique_ptr<Scene> SceneFactory::createSceneFromYaml(
    const std::string& yamlPath, Screen scr) {
  return SceneLoader::loadFromFile(yamlPath, scr);
}

void SceneFactory::scanYamlScenes(const std::string& directory) {
  yamlPaths_.clear();
  yamlNames_.clear();

  namespace fs = std::filesystem;
  if (!fs::exists(directory) || !fs::is_directory(directory))
    return;

  std::vector<fs::directory_entry> entries;
  for (const auto& entry : fs::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      auto ext = entry.path().extension().string();
      if (ext == ".yaml" || ext == ".yml")
        entries.push_back(entry);
    }
  }

  std::sort(entries.begin(), entries.end(),
            [](const fs::directory_entry& a, const fs::directory_entry& b) {
              return a.path().filename() < b.path().filename();
            });

  for (const auto& entry : entries) {
    yamlPaths_.push_back(entry.path().string());
    yamlNames_.push_back(entry.path().stem().string());
  }

  scanned_ = true;
  buildCombinedNames();
}

void SceneFactory::buildCombinedNames() {
  constexpr int builtinCount = static_cast<int>(SceneType::BUILTIN_COUNT);
  combinedNames_.clear();
  combinedNames_.reserve(builtinCount + yamlNames_.size());

  for (int i = 0; i < builtinCount; ++i)
    combinedNames_.push_back(sceneNames[i]);

  for (const auto& name : yamlNames_)
    combinedNames_.push_back(name);
}

const std::vector<std::string>& SceneFactory::allSceneNames() {
  if (!scanned_)
    scanYamlScenes("resources/scenes");
  return combinedNames_;
}

int SceneFactory::sceneCount() {
  return static_cast<int>(allSceneNames().size());
}

std::unique_ptr<Scene> SceneFactory::createSceneByIndex(int index, Screen scr) {
  constexpr int builtinCount = static_cast<int>(SceneType::BUILTIN_COUNT);

  if (index < builtinCount)
    return createScene(static_cast<SceneType>(index), scr);

  int yamlIndex = index - builtinCount;
  if (!scanned_)
    scanYamlScenes("resources/scenes");

  if (yamlIndex >= 0 && yamlIndex < static_cast<int>(yamlPaths_.size()))
    return createSceneFromYaml(yamlPaths_[yamlIndex], scr);

  return nullptr;
}
