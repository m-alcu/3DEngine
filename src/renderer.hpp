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
#include "objects/solid.hpp"
#include "rasterizer.hpp"
#include <cstdint>

class Renderer {

public:
  void drawScene(Scene &scene) {

    // Shadow pass - render depth from light's perspective
    if (scene.shadowMap && scene.shadowsEnabled) {
      renderShadowPass(scene);
    }

    scene.drawBackground();

    prepareFrame(scene);
    for (auto &solidPtr : scene.solids) {
      switch (solidPtr->shading) {
      case Shading::Flat:
        flatRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::Wireframe:
        flatRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::TexturedFlat:
        texturedFlatRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::Gouraud:
        gouraudRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::TexturedGouraud:
        texturedGouraudRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::BlinnPhong:
        blinnPhongRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::TexturedBlinnPhong:
        texturedBlinnPhongRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::Phong:
        phongRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      case Shading::TexturedPhong:
        texturedPhongRasterizer.drawRenderable(*solidPtr, &scene);
        break;
      default:
        flatRasterizer.drawRenderable(*solidPtr, &scene);
      }
    }

    if (scene.showShadowMapOverlay) {
      drawShadowMapOverlay(scene);
    }
  }

  void renderShadowPass(Scene &scene) {
    scene.shadowMap->clear();

    // Calculate dynamic scene bounds (AABB in world space)
    slib::vec3 minV{std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()};
    slib::vec3 maxV{-std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max()};
    bool hasGeometry = false;

    for (auto &solidPtr : scene.solids) {
      if (solidPtr->lightSourceEnabled) {
        continue; // Skip light sources
      }
      solidPtr->updateWorldBounds(minV, maxV);
      hasGeometry = true;
    }

    // Fallback in case of no geometry
    slib::vec3 sceneCenter;
    float sceneRadius;
    if (hasGeometry) {
      sceneCenter =
          slib::vec3{(minV.x + maxV.x) * 0.5f, (minV.y + maxV.y) * 0.5f,
                     (minV.z + maxV.z) * 0.5f};
      slib::vec3 diag{maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z};
      float diagLen2 = smath::dot(diag, diag);
      sceneRadius = 0.5f * std::sqrt(diagLen2);
      // Padding factor to avoid clipping at frustum edges
      // const float paddingFactor = 1.25f; // increase if needed
      // sceneRadius *= paddingFactor;
      // Ensure a minimal radius to avoid degenerate projections
      sceneRadius = std::max(sceneRadius, 1.0f);
    } else {
      sceneCenter = {0.0f, 0.0f, -400.0f};
      sceneRadius = 125.0f;
    }

    // Build light matrices
    scene.shadowMap->buildLightMatrices(scene.light, sceneCenter, sceneRadius);

    // Render all shadow-casting solids to the shadow map
    for (auto &solidPtr : scene.solids) {
      // Skip light sources - they don't cast shadows on themselves
      if (!solidPtr->lightSourceEnabled) {
        shadowRasterizer.drawRenderable(*solidPtr, &scene,
                                        scene.shadowMap.get());
      }
    }
  }

  void prepareFrame(Scene &scene) {

    // std::fill_n(scene.pixels, scene.screen.width * scene.screen.height, 0);
    std::copy(scene.backg,
              scene.backg + scene.screen.width * scene.screen.height,
              scene.pixels);
    scene.zBuffer->Clear(); // Clear the zBuffer

    float aspectRatio =
        (float)scene.screen.width / scene.screen.height; // Width / Height ratio
    float fovRadians = scene.camera.viewAngle * (PI / 180.0f);

    slib::mat4 projectionMatrix =
        smath::perspective(scene.camera.zFar, scene.camera.zNear, aspectRatio, fovRadians);

    slib::mat4 viewMatrix =
        scene.orbiting ? smath::lookAt(scene.camera.pos,
                                       scene.camera.orbitTarget, {0, 1, 0})
                       : smath::fpsview(scene.camera.pos, scene.camera.pitch,
                                        scene.camera.yaw, scene.camera.roll);
    scene.spaceMatrix = viewMatrix * projectionMatrix;

    // Used in BlinnPhong shading
    // NOTE: For performance we approximate the per-fragment view vector V with
    // -camera.forward. This assumes all view rays are parallel (like an
    // orthographic camera). Works well when the camera is far away or objects
    // are small on screen. Not physically correct: highlights will "stick" to
    // the camera instead of sliding across surfaces when moving in perspective,
    // but it�s often a good enough approximation.d
    scene.forwardNeg = {-scene.camera.forward.x, -scene.camera.forward.y,
                        -scene.camera.forward.z};
  }

  // Draw the shadow map as a small overlay in the corner of the screen
  void drawShadowMapOverlay(Scene &scene, int overlaySize = SHADOW_MAP_OVERVIEW_SIZE,
                            int margin = 10) {
    if (!scene.shadowMap || !scene.shadowsEnabled)
      return;

    const ShadowMap &sm = *scene.shadowMap;

    // Position in bottom-left corner
    int startX = margin;
    int startY = scene.screen.height - overlaySize - margin;

    // Find min/max depth for normalization (excluding max float values)
    float minDepth = 1.0f;
    float maxDepth = -1.0f;

    for (int i = 0; i < sm.width * sm.height; ++i) {
      float d = sm.getDepth(i);
      minDepth = std::min(minDepth, d);
      maxDepth = std::max(maxDepth, d);
    }

    // Avoid division by zero
    float depthRange = maxDepth - minDepth;
    if (depthRange < 0.0001f)
      depthRange = 1.0f;

    // Scale factors for sampling the shadow map
    float scaleX = static_cast<float>(sm.width) / overlaySize;
    float scaleY = static_cast<float>(sm.height) / overlaySize;

    for (int y = 0; y < overlaySize; ++y) {
      for (int x = 0; x < overlaySize; ++x) {
        int screenX = startX + x;
        int screenY = startY + y;

        // Bounds check
        if (screenX < 0 || screenX >= scene.screen.width || screenY < 0 ||
            screenY >= scene.screen.height) {
          continue;
        }

        // Sample from shadow map
        int smX = static_cast<int>(x * scaleX);
        int smY = static_cast<int>(y * scaleY);
        smX = std::clamp(smX, 0, sm.width - 1);
        smY = std::clamp(smY, 0, sm.height - 1);

        float depth = sm.getDepth(smY * sm.width + smX);

        uint8_t gray = 0;
        if (depth < 1.0f) {
          // Normalize depth to 255-0 (inverse: closer = brighter)
          float normalized = (maxDepth - depth) / depthRange;
          gray = static_cast<uint8_t>(
              std::clamp(normalized * 255.0f, 0.0f, 255.0f));
        }

        // ARGB format
        uint32_t color = (255 << 24) | (gray << 16) | (gray << 8) | gray;
        scene.pixels[screenY * scene.screen.width + screenX] = color;
      }
    }

    // Draw border around the overlay
    uint32_t borderColor =
        (255 << 24) | (255 << 16) | (255 << 8) | 255; // White border
    for (int x = 0; x < overlaySize; ++x) {
      int topY = startY;
      int bottomY = startY + overlaySize - 1;
      if (startX + x >= 0 && startX + x < scene.screen.width) {
        if (topY >= 0 && topY < scene.screen.height)
          scene.pixels[topY * scene.screen.width + startX + x] = borderColor;
        if (bottomY >= 0 && bottomY < scene.screen.height)
          scene.pixels[bottomY * scene.screen.width + startX + x] = borderColor;
      }
    }
    for (int y = 0; y < overlaySize; ++y) {
      int leftX = startX;
      int rightX = startX + overlaySize - 1;
      if (startY + y >= 0 && startY + y < scene.screen.height) {
        if (leftX >= 0 && leftX < scene.screen.width)
          scene.pixels[(startY + y) * scene.screen.width + leftX] = borderColor;
        if (rightX >= 0 && rightX < scene.screen.width)
          scene.pixels[(startY + y) * scene.screen.width + rightX] =
              borderColor;
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
  Rasterizer<ShadowEffect> shadowRasterizer;
};
