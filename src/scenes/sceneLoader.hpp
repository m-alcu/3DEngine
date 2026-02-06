#pragma once

#include <memory>
#include <string>
#include "../scene.hpp"

namespace YAML { class Node; }

class SceneLoader {
public:
    static std::unique_ptr<Scene> loadFromFile(const std::string& yamlPath,
                                                Screen scr);

private:
    static Shading parseShading(const std::string& str);
    static LightType parseLightType(const std::string& str);
    static BackgroundType parseBackgroundType(const std::string& str);

    static std::unique_ptr<Solid> parseSolid(const YAML::Node& solidNode);
    static void parseCamera(const YAML::Node& cameraNode, Camera& camera);
    static void parseLight(const YAML::Node& lightNode, Light& light);
    static void parseOrbit(const YAML::Node& orbitNode, Solid& solid);
    static void parsePosition(const YAML::Node& solidNode, Solid& solid);
};
