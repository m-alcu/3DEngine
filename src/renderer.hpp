#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include "effects/flat_effect.hpp"
#include "effects/gouraud_effect.hpp"
#include "effects/phong_effect.hpp"
#include "effects/shadow_effect.hpp"
#include "effects/textured_flat_effect.hpp"
#include "effects/textured_gouraud_effect.hpp"
#include "effects/textured_phong_effect.hpp"
#include "effects/environment_map_effect.hpp"
#include "rasterizer/rasterizer.hpp"
#include "rasterizer/rasterizer_shadow.hpp"
#include "ecs/shadow_system.hpp"
#include "renderer_axis.hpp"
#include "renderer_overlay.hpp"
#include "renderer_fonts.hpp"

class Renderer {

public:
  Renderer() {
    rasterizers[static_cast<size_t>(Shading::Wireframe)]       = std::make_unique<Rasterizer<FlatEffect>>();
    rasterizers[static_cast<size_t>(Shading::Flat)]            = std::make_unique<Rasterizer<FlatEffect>>();
    rasterizers[static_cast<size_t>(Shading::Gouraud)]         = std::make_unique<Rasterizer<GouraudEffect>>();
    rasterizers[static_cast<size_t>(Shading::Phong)]           = std::make_unique<Rasterizer<PhongEffect>>();
    rasterizers[static_cast<size_t>(Shading::TexturedFlat)]    = std::make_unique<Rasterizer<TexturedFlatEffect>>();
    rasterizers[static_cast<size_t>(Shading::TexturedGouraud)] = std::make_unique<Rasterizer<TexturedGouraudEffect>>();
    rasterizers[static_cast<size_t>(Shading::TexturedPhong)]   = std::make_unique<Rasterizer<TexturedPhongEffect>>();
    rasterizers[static_cast<size_t>(Shading::EnvironmentMap)]  = std::make_unique<Rasterizer<EnvironmentMapEffect>>();
  }

  void drawScene(Scene &scene) {

    scene.zBuffer->Clear(); // Clear the zBuffer
    scene.stats.reset();

    float aspectRatio = (float)scene.screen.width / scene.screen.height;
    scene.spaceMatrix = scene.camera.viewMatrix(scene.orbiting) * scene.camera.projectionMatrix(aspectRatio);

    // Shadow pass - render depth from light's perspective for each light source
    if (scene.shadowsEnabled) {
      renderLightMaps(scene);
    }

    scene.drawBackground();

    if (scene.showAxes) {
      RendererAxis::drawAxes(scene);
    }

    if (!scene.name.empty()) {
      int textWidth = static_cast<int>(scene.name.size()) * RendererFonts::getGlyphWidth(scene.font);
      int tx = scene.screen.width - textWidth - 10;
      int ty = scene.screen.height - 18;
      RendererFonts::drawText(scene.pixels.data(), scene.screen.width, scene.screen.height,
                              scene.screen.width, tx, ty, scene.name.c_str(),
                              0xFFFFFFFFu, 0xFF000000u, true, scene.font);
    }

    for (const auto& [entity, render] : scene.registry.renders()) {
      auto* transform = scene.registry.transforms().get(entity);
      auto* mesh = scene.registry.meshes().get(entity);
      auto* material = scene.registry.materials().get(entity);
      if (!transform || !mesh || !material) {
        continue;
      }

      auto idx = static_cast<size_t>(render.shading);
      IRasterizer* rasterizer = (idx < rasterizers.size() && rasterizers[idx])
                                  ? rasterizers[idx].get()
                                  : rasterizers[static_cast<size_t>(Shading::Flat)].get();
      rasterizer->drawRenderable(*transform, *mesh, *material, render.shading, &scene);
    }

    if (scene.showShadowMapOverlay) {
      RendererOverlay::drawShadowMapOverlay(scene, 10);
    }
  }

  void renderLightMaps(Scene &scene) {
    for (const auto& [lightEntity, lightComponent] : scene.registry.lights()) {
      auto* shadowComponent = scene.registry.shadows().get(lightEntity);
      if (!shadowComponent || !shadowComponent->shadowMap) {
        continue;
      }

      ShadowMap& shadowMap = *shadowComponent->shadowMap;
      ShadowSystem::buildLightMatrices(shadowMap, lightComponent.light,
                                       scene.sceneCenter, scene.sceneRadius);
      shadowMap.clearAllFaces();

      for (const auto& [entity, render] : scene.registry.renders()) {
        auto* transform = scene.registry.transforms().get(entity);
        auto* mesh = scene.registry.meshes().get(entity);
        if (!transform || !mesh) {
          continue;
        }
        shadowRasterizer.drawRenderable(*transform, *mesh, lightComponent, *shadowComponent);
      }
    }
  }

  std::array<std::unique_ptr<IRasterizer>, 8> rasterizers;
  ShadowRasterizer<ShadowEffect> shadowRasterizer;
};
