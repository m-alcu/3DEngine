#pragma once

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
#include "axis_renderer.hpp"
#include "bresenham.hpp"
#include "rasterizer.hpp"
#include "rasterizer_shadow.hpp"
#include <cstdint>
#include <cstdio>
#include "fonts.hpp"
#include "ecs/shadow_system.hpp"

class Renderer {

public:
  void drawScene(Scene &scene) {

    prepareFrame(scene);
    if (scene.showAxes) {
      AxisRenderer::drawAxes(scene);
    }

    // Shadow pass - render depth from light's perspective for each light source
    if (scene.shadowsEnabled) {
      renderShadowPass(scene);
    }

    scene.drawBackground();

    if (!scene.name.empty()) {
      int textWidth = static_cast<int>(scene.name.size()) * Font8x8::getGlyphWidth(scene.font);
      int tx = scene.screen.width - textWidth - 10;
      int ty = scene.screen.height - 18;
      Font8x8::drawText(scene.pixels, scene.screen.width, scene.screen.height,
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
      drawShadowMapOverlay(scene);
    }
  }

  void renderShadowPass(Scene &scene) {
    for (Entity lightEntity : scene.lightSourceEntities()) {
      auto* lightComponent = scene.registry.lights().get(lightEntity);
      auto* shadowComponent = scene.registry.shadows().get(lightEntity);
      if (!lightComponent || !shadowComponent || !shadowComponent->shadowMap) {
        continue;
      }
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
          auto* render = scene.registry.renders().get(entity);
          if (!transform || !mesh || !render) {
            continue;
          }
          shadowRasterizer.drawRenderable(*transform, *mesh,
                                          lightComponent, shadowComponent,
                                          faceIdx);
        }
      }
    }
  }

  void prepareFrame(Scene &scene) {

    // std::fill_n(scene.pixels, scene.screen.width * scene.screen.height, 0);
    std::copy(scene.backg,
              scene.backg + scene.screen.width * scene.screen.height,
              scene.pixels);
    scene.zBuffer->Clear(); // Clear the zBuffer
    scene.stats.reset();

    float aspectRatio =
        (float)scene.screen.width / scene.screen.height; // Width / Height ratio
    float fovRadians = scene.camera.viewAngle * RAD;

    slib::mat4 projectionMatrix =
        smath::perspective(scene.camera.zFar, scene.camera.zNear, aspectRatio, fovRadians);

    if (scene.orbiting) {
      slib::vec3 up = {0.0f, 1.0f, 0.0f};
      slib::vec3 lightDir = smath::normalize(scene.camera.orbitTarget - scene.camera.pos);
      // gimbal lock avoidance check https://en.wikipedia.org/wiki/Gimbal_lock
      if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
        up = {1.0f, 0.0f, 0.0f};
      }
      scene.spaceMatrix = smath::lookAt(scene.camera.pos, scene.camera.orbitTarget, up) * projectionMatrix;
    } else {
      scene.spaceMatrix = smath::fpsview(scene.camera.pos, scene.camera.pitch, scene.camera.yaw, scene.camera.roll) * projectionMatrix;
    }

    scene.forwardNeg = {-scene.camera.forward.x, -scene.camera.forward.y,
                        -scene.camera.forward.z};
  }

  // Draw shadow map overlay - handles both single-face and cubemap
  void drawShadowMapOverlay(Scene &scene, int margin = 10) {
    if (!scene.shadowsEnabled)
      return;

    ShadowMap* shadowMapPtr = findShadowMapForOverlay(scene);
    if (!shadowMapPtr)
      return;

    if (shadowMapPtr->isCubemap()) {
      // Draw 6 cubemap faces in a row
      int faceOverlaySize = SHADOW_MAP_OVERVIEW_SIZE / 2;
      static const char* faceLabels[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
      int startY = scene.screen.height - faceOverlaySize - margin;

      for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
        int startX = margin + faceIdx * (faceOverlaySize + 2);
        shadowMapPtr->drawFaceOverlay(faceIdx, scene.pixels, scene.screen.width,
                                       scene.screen.height, startX, startY, faceOverlaySize);
        uint32_t labelColor = shadowMapPtr->faceDirty[faceIdx] ? RED_COLOR : WHITE_COLOR;
        Font8x8::drawText(scene.pixels, scene.screen.width, scene.screen.height,
                          scene.screen.width, startX + 2, startY + 2,
                          faceLabels[faceIdx], labelColor, BLACK_COLOR, true, scene.font);
      }
    } else {
      // Single face overlay
      int overlaySize = SHADOW_MAP_OVERVIEW_SIZE;
      int startX = margin;
      int startY = scene.screen.height - overlaySize - margin;
      shadowMapPtr->drawOverlay(scene.pixels, scene.screen.width, scene.screen.height,
                                startX, startY, overlaySize);
    }
  }

private:
  ShadowMap* findShadowMapForOverlay(Scene& scene) {
    // Check selected entity first
    if (scene.selectedEntityIndex >= 0 &&
        scene.selectedEntityIndex < static_cast<int>(scene.entities.size())) {
      Entity selectedEntity = scene.entities[scene.selectedEntityIndex];
      auto* shadowComponent = scene.registry.shadows().get(selectedEntity);
      if (shadowComponent && shadowComponent->shadowMap) {
        return shadowComponent->shadowMap.get();
      }
    }

    // Fall back to first light source entity's shadow map
    for (const auto& [entity, shadowComponent] : scene.registry.shadows()) {
      if (shadowComponent.shadowMap) {
        return shadowComponent.shadowMap.get();
      }
    }

    return nullptr;
  }

public:
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
