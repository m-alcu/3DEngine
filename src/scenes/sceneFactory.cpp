#include "sceneFactory.hpp"
#include "amigaScene.hpp"
#include "cubeScene.hpp"
#include "icosahedronScene.hpp"
#include "knotScene.hpp"
#include "starScene.hpp"
#include "tetrakisScene.hpp"
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
		case SceneType::STAR:
			return std::make_unique<StarScene>(scr);
		case SceneType::TETRAKIS:
			return std::make_unique<TetrakisScene>(scr);
		case SceneType::TORUS:
			return std::make_unique<TorusScene>(scr);
		case SceneType::WORLD:
			return std::make_unique<WorldScene>(scr);
        default:
            return nullptr;
    }
}
