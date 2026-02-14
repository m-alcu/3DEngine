#include "sceneFactory.hpp"
#include "sceneLoader.hpp"

#include <algorithm>
#include <filesystem>

// Static member definitions
std::vector<std::string> SceneFactory::yamlPaths_;
std::vector<std::string> SceneFactory::yamlNames_;
std::vector<std::string> SceneFactory::combinedNames_;
bool SceneFactory::scanned_ = false;

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
  combinedNames_.clear();
  combinedNames_.reserve(yamlNames_.size());

  for (const auto& name : yamlNames_)
    combinedNames_.push_back(name);
}

const std::vector<std::string>& SceneFactory::allSceneNames() {
  if (!scanned_)
    scanYamlScenes(SCENES_PATH);
  return combinedNames_;
}

int SceneFactory::sceneCount() {
  return static_cast<int>(allSceneNames().size());
}

std::unique_ptr<Scene> SceneFactory::createSceneByIndex(int index, Screen scr) {
  if (!scanned_)
    scanYamlScenes(SCENES_PATH);

  if (index >= 0 && index < static_cast<int>(yamlPaths_.size()))
    return createSceneFromYaml(yamlPaths_[index], scr);

  return nullptr;
}
