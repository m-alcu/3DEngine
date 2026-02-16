#pragma once

#include "effects/blinn_phong_effect.hpp"
#include "effects/flat_effect.hpp"
#include "effects/gouraud_effect.hpp"
#include "effects/phong_effect.hpp"
#include "effects/shadow_effect.hpp"
#include "effects/cubemap_shadow_effect.hpp"
#include "effects/textured_blinn_phong_effect.hpp"
#include "effects/textured_flat_effect.hpp"
#include "effects/textured_gouraud_effect.hpp"
#include "effects/textured_phong_effect.hpp"
#include "effects/environment_map_effect.hpp"
#include "axis_renderer.hpp"
#include "bresenham.hpp"
#include "rasterizer.hpp"
#include "rasterizer_shadow.hpp"
#include "rasterizer_cubemap_shadow.hpp"
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
      if (scene.useCubemapShadows) {
        drawCubemapShadowMapOverlay(scene);
      } else {
        drawShadowMapOverlay(scene);
      }
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
                                       scene.sceneRadius,
                                       scene.minBiasDefault,
                                       scene.maxBiasDefault,
                                       scene.shadowBiasMin,
                                       scene.shadowBiasMax);

      // Check if this is a point light with cubemap shadows
      bool useCubemap = shadowComponent->shadowMap->useCubemap && 
                       shadowComponent->shadowMap->cubeShadowMap &&
                       lightComponent->light.type == LightType::Point;

      if (useCubemap) {
        // Render to all 6 cubemap faces
        for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
          CubeShadowFace face = static_cast<CubeShadowFace>(faceIdx);
          
          for (Entity entity : scene.renderableEntities()) {
            auto* transform = scene.registry.transforms().get(entity);
            auto* mesh = scene.registry.meshes().get(entity);
            auto* render = scene.registry.renders().get(entity);
            if (!transform || !mesh || !render) {
              continue;
            }
            cubemapShadowRasterizer.drawRenderable(*transform,
                                                   *mesh,
                                                   lightComponent,
                                                   shadowComponent,
                                                   face);
          }
        }
      } else {
        // Regular 2D shadow map rendering
        for (Entity entity : scene.renderableEntities()) {
          auto* transform = scene.registry.transforms().get(entity);
          auto* mesh = scene.registry.meshes().get(entity);
          auto* render = scene.registry.renders().get(entity);
          if (!transform || !mesh || !render) {
            continue;
          }
    shadowRasterizer.drawRenderable(*transform,
                                          *mesh,
                                          lightComponent,
                                          shadowComponent);
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

    // Used in Phong shading
    // NOTE: For performance we approximate the per-fragment view vector V with
    // -camera.forward. This assumes all view rays are parallel (like an
    // orthographic camera). Works well when the camera is far away or objects
    // are small on screen. Not physically correct: highlights will "stick" to
    // the camera instead of sliding across surfaces when moving in perspective,
    // but it’s often a good enough approximation.
    scene.forwardNeg = {-scene.camera.forward.x, -scene.camera.forward.y,
                        -scene.camera.forward.z};
  }

  // Draw the shadow map as a small overlay in the corner of the screen
  void drawShadowMapOverlay(Scene &scene, int overlaySize = SHADOW_MAP_OVERVIEW_SIZE,
                            int margin = 10) {
    if (!scene.shadowsEnabled)
      return;

    // Use shadow map from selected entity if it has one
    ShadowMap* shadowMapPtr = nullptr;
    if (scene.selectedEntityIndex >= 0 &&
        scene.selectedEntityIndex < static_cast<int>(scene.entities.size())) {
      Entity selectedEntity = scene.entities[scene.selectedEntityIndex];
      auto* shadowComponent = scene.registry.shadows().get(selectedEntity);
      if (shadowComponent && shadowComponent->shadowMap) {
        shadowMapPtr = shadowComponent->shadowMap.get();
      }
    }

    // Fall back to first light source entity's shadow map
    if (!shadowMapPtr) {
      for (const auto& [entity, shadowComponent] : scene.registry.shadows()) {
        if (shadowComponent.shadowMap) {
          shadowMapPtr = shadowComponent.shadowMap.get();
          break;
        }
      }
    }

    if (!shadowMapPtr)
      return;

    int startX = margin;
    int startY = scene.screen.height - overlaySize - margin;
    shadowMapPtr->drawOverlay(scene.pixels, scene.screen.width, scene.screen.height,
                              startX, startY, overlaySize);

  }

  // Draw the 6 cubemap shadow faces as small overlays in a row
  void drawCubemapShadowMapOverlay(Scene &scene, int faceOverlaySize = 100,
                                    int margin = 10) {
    if (!scene.shadowsEnabled)
      return;

    // Find a cubemap shadow map to display
    CubeShadowMap* cubeShadowPtr = nullptr;
    if (scene.selectedEntityIndex >= 0 &&
        scene.selectedEntityIndex < static_cast<int>(scene.entities.size())) {
      Entity selectedEntity = scene.entities[scene.selectedEntityIndex];
      auto* shadowComponent = scene.registry.shadows().get(selectedEntity);
      if (shadowComponent && shadowComponent->shadowMap &&
          shadowComponent->shadowMap->cubeShadowMap) {
        cubeShadowPtr = shadowComponent->shadowMap->cubeShadowMap.get();
      }
    }

    if (!cubeShadowPtr) {
      for (const auto& [entity, shadowComponent] : scene.registry.shadows()) {
        if (shadowComponent.shadowMap && shadowComponent.shadowMap->cubeShadowMap) {
          cubeShadowPtr = shadowComponent.shadowMap->cubeShadowMap.get();
          break;
        }
      }
    }

    if (!cubeShadowPtr)
      return;

    static const char* faceLabels[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
    int startY = scene.screen.height - faceOverlaySize - margin;

    for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
      auto& zb = cubeShadowPtr->faces[faceIdx];
      if (!zb) continue;

      int faceSize = cubeShadowPtr->faceSize;
      int startX = margin + faceIdx * (faceOverlaySize + 2);

      // Find min/max depth for normalization
      float minDepth = 1.0f;
      float maxDepth = -1.0f;
      for (int i = 0; i < faceSize * faceSize; ++i) {
        float d = zb->Get(i);
        minDepth = std::min(minDepth, d);
        maxDepth = std::max(maxDepth, d);
      }
      float depthRange = std::max(maxDepth - minDepth, 0.0001f);

      blitScaled(scene.pixels, scene.screen.width, scene.screen.height,
                 startX, startY, faceOverlaySize, faceOverlaySize,
                 faceSize, faceSize,
                 [&](int srcX, int srcY) -> uint32_t {
                   float depth = zb->Get(srcY * faceSize + srcX);
                   uint8_t gray = (depth < 1.0f)
                       ? static_cast<uint8_t>(std::clamp((maxDepth - depth) / depthRange * 255.0f, 0.0f, 255.0f))
                       : 0;
                   return Color(gray, gray, gray).toBgra();
                 });

      // Border
      int endX = startX + faceOverlaySize - 1;
      int endY = startY + faceOverlaySize - 1;
      drawBresenhamLine(startX, startY, endX, startY, scene.pixels, WHITE_COLOR, scene.screen.width, scene.screen.height);
      drawBresenhamLine(startX, endY, endX, endY, scene.pixels, WHITE_COLOR, scene.screen.width, scene.screen.height);
      drawBresenhamLine(startX, startY, startX, endY, scene.pixels, WHITE_COLOR, scene.screen.width, scene.screen.height);
      drawBresenhamLine(endX, startY, endX, endY, scene.pixels, WHITE_COLOR, scene.screen.width, scene.screen.height);

      // Face label
      Font8x8::drawText(scene.pixels, scene.screen.width, scene.screen.height,
                        scene.screen.width, startX + 2, startY + 2,
                        faceLabels[faceIdx], WHITE_COLOR, BLACK_COLOR, true, scene.font);
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
  CubemapShadowRasterizer<CubemapShadowEffect> cubemapShadowRasterizer;
};
