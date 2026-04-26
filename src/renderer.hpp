#pragma once

#include <cstdint>
#include <cstdio>
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

      switch (render.shading) {
      case Shading::Flat:
      case Shading::Wireframe:
        flatRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      case Shading::TexturedFlat:
        texturedFlatRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      case Shading::Gouraud:
        gouraudRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      case Shading::TexturedGouraud:
        texturedGouraudRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      case Shading::Phong:
        phongRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      case Shading::TexturedPhong:
        texturedPhongRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      case Shading::EnvironmentMap:
        environmentMapRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
        break;
      default:
        flatRasterizer.drawRenderable(*transform, *mesh, *material, render.shading, &scene);
      }
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

      shadowComponent->shadowMap->setAllDirty();

      ShadowSystem::buildLightMatrices(*shadowComponent,
                                       lightComponent.light,
                                       scene.sceneCenter,
                                       scene.sceneRadius);

      int numFaces = shadowComponent->shadowMap->numFaces;

      for (const auto& [entity, render] : scene.registry.renders()) {
        auto* transform = scene.registry.transforms().get(entity);
        auto* mesh = scene.registry.meshes().get(entity);
        if (!transform || !mesh) {
          continue;
        }
        for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx) {
          shadowRasterizer.drawRenderable(*transform, *mesh,
                                          lightComponent, *shadowComponent,
                                          faceIdx);
        }
      }
    }
  }

  Rasterizer<FlatEffect> flatRasterizer;
  Rasterizer<GouraudEffect> gouraudRasterizer;
  Rasterizer<PhongEffect> phongRasterizer;
  Rasterizer<TexturedFlatEffect> texturedFlatRasterizer;
  Rasterizer<TexturedGouraudEffect> texturedGouraudRasterizer;
  Rasterizer<TexturedPhongEffect> texturedPhongRasterizer;
  Rasterizer<EnvironmentMapEffect> environmentMapRasterizer;
  ShadowRasterizer<ShadowEffect> shadowRasterizer;
};
