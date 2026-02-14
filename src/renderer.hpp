#pragma once

#include "effects/blinnPhongEffect.hpp"
#include "effects/flatEffect.hpp"
#include "effects/gouraudEffect.hpp"
#include "effects/phongEffect.hpp"
#include "effects/shadowEffect.hpp"
#include "effects/texturedBlinnPhongEffect.hpp"
#include "effects/texturedFlatEffect.hpp"
#include "effects/texturedGouraudEffect.hpp"
#include "effects/texturedPhongEffect.hpp"
#include "effects/environmentMapEffect.hpp"
#include "axisRenderer.hpp"
#include "bresenham.hpp"
#include "rasterizer.hpp"
#include "rasterizer_shadow.hpp"
#include <cstdint>
#include "fonts.hpp"
#include "ecs/ShadowSystem.hpp"

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
      int textWidth = static_cast<int>(scene.name.size()) * 8;
      int tx = scene.screen.width - textWidth - 10;
      int ty = scene.screen.height - 18;
      Font8x8::drawText(scene.pixels, scene.screen.width, scene.screen.height,
                        scene.screen.width, tx, ty, scene.name.c_str(),
                        0xFFFFFFFFu, 0xFF000000u, true);
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
