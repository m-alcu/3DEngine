#pragma once

#include <cstdint>
#include <cstdio>
#include "effects/blinn_phong_effect.hpp"
#include "effects/flat_effect.hpp"
#include "effects/gouraud_effect.hpp"
#include "effects/phong_effect.hpp"
#include "effects/shadow_effect.hpp"
#include "effects/textured_blinn_phong_effect.hpp"
#include "effects/textured_flat_effect.hpp"
#include "effects/textured_gouraud_effect.hpp"
#include "effects/textured_phong_effect.hpp"
#include "effects/environment_map_effect.hpp"
#include "rasterizer.hpp"
#include "rasterizer_shadow.hpp"
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
      renderShadowPass(scene);
    }

    scene.drawBackground();

    if (scene.showAxes) {
      RendererAxis::drawAxes(scene);
    }

    if (!scene.name.empty()) {
      int textWidth = static_cast<int>(scene.name.size()) * RendererFonts::getGlyphWidth(scene.font);
      int tx = scene.screen.width - textWidth - 10;
      int ty = scene.screen.height - 18;
      RendererFonts::drawText(scene.pixels, scene.screen.width, scene.screen.height,
                              scene.screen.width, tx, ty, scene.name.c_str(),
                              0xFFFFFFFFu, 0xFF000000u, true, scene.font);
    }

    for (Entity entity : scene.renderableEntities()) {
      auto* transform = scene.registry.transforms().get(entity);
      auto* mesh = scene.registry.meshes().get(entity);
      auto* material = scene.registry.materials().get(entity);
      auto* render = scene.registry.renders().get(entity);
      if (!transform || !mesh || !material || !render) {
        continue;
      }

      switch (render->shading) {
      case Shading::Flat:
        flatRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::Wireframe:
        flatRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::TexturedFlat:
        texturedFlatRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::Gouraud:
        gouraudRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::TexturedGouraud:
        texturedGouraudRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::BlinnPhong:
        blinnPhongRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::TexturedBlinnPhong:
        texturedBlinnPhongRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::Phong:
        phongRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::TexturedPhong:
        texturedPhongRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      case Shading::EnvironmentMap:
        environmentMapRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
        break;
      default:
        flatRasterizer.drawRenderable(*transform, *mesh, *material, render->shading, &scene);
      }
    }

    if (scene.showShadowMapOverlay) {
      RendererOverlay::drawShadowMapOverlay(scene, 10);
    }
  }

  void renderShadowPass(Scene &scene) {
    for (Entity lightEntity : scene.lightSourceEntities()) {
      auto* lightComponent = scene.registry.lights().get(lightEntity);
      auto* shadowComponent = scene.registry.shadows().get(lightEntity);
      if (!lightComponent || !shadowComponent || !shadowComponent->shadowMap) {
        continue;
      }

      //setting all faces as dirty so they will be cleared before rendering if needed
      shadowComponent->shadowMap->clear(); 
      
      ShadowSystem::buildLightMatrices(*shadowComponent,
                                       lightComponent->light,
                                       scene.sceneCenter,
                                       scene.sceneRadius);

      int numFaces = shadowComponent->shadowMap->numFaces;

      for (int faceIdx = 0; faceIdx < numFaces; ++faceIdx) {
        for (Entity entity : scene.renderableEntities()) {
          auto* transform = scene.registry.transforms().get(entity);
          auto* mesh = scene.registry.meshes().get(entity);
          if (!transform || !mesh) {
            continue;
          }
          shadowRasterizer.drawRenderable(*transform, *mesh,
                                          lightComponent, shadowComponent,
                                          faceIdx);
        }
      }
    }
  }

  Rasterizer<FlatEffect> flatRasterizer;
  Rasterizer<GouraudEffect> gouraudRasterizer;
  Rasterizer<PhongEffect> phongRasterizer;
  Rasterizer<BlinnPhongEffect> blinnPhongRasterizer;
  Rasterizer<TexturedFlatEffect> texturedFlatRasterizer;
  Rasterizer<TexturedGouraudEffect> texturedGouraudRasterizer;
  Rasterizer<TexturedPhongEffect> texturedPhongRasterizer;
  Rasterizer<TexturedBlinnPhongEffect> texturedBlinnPhongRasterizer;
  Rasterizer<EnvironmentMapEffect> environmentMapRasterizer;
  ShadowRasterizer<ShadowEffect> shadowRasterizer;
};
