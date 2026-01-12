#pragma once

#include "ShadowRasterizer.hpp"
#include "effects/blinnPhongEffect.hpp"
#include "effects/flatEffect.hpp"
#include "effects/gouraudEffect.hpp"
#include "effects/phongEffect.hpp"
#include "effects/texturedBlinnPhongEffect.hpp"
#include "effects/texturedFlatEffect.hpp"
#include "effects/texturedGouraudEffect.hpp"
#include "effects/texturedPhongEffect.hpp"
#include "objects/solid.hpp"
#include "rasterizer.hpp"
#include <cstdint>
#include <iostream>

class Renderer {

public:
  void drawScene(Scene &scene, float zNear, float zFar, float viewAngle) {

    // Shadow pass - render depth from light's perspective
    if (scene.shadowMap && scene.shadowsEnabled) {
      renderShadowPass(scene);
    }

    scene.drawBackground();

    prepareFrame(scene, zNear, zFar, viewAngle);
    for (auto &solidPtr : scene.solids) {
      switch (solidPtr->shading) {
      case Shading::Flat:
        flatRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::Wireframe:
        flatRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::TexturedFlat:
        texturedFlatRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::Gouraud:
        gouraudRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::TexturedGouraud:
        texturedGouraudRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::BlinnPhong:
        blinnPhongRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::TexturedBlinnPhong:
        texturedBlinnPhongRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::Phong:
        phongRasterizer.drawRenderable(*solidPtr, scene);
        break;
      case Shading::TexturedPhong:
        texturedPhongRasterizer.drawRenderable(*solidPtr, scene);
        break;
      default:
        flatRasterizer.drawRenderable(*solidPtr, scene);
      }
    }
  }

  void renderShadowPass(Scene &scene) {
    scene.shadowMap->clear();

    // Calculate dynamic scene bounds (AABB in world space)
    slib::vec3 minV{ std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max(),
                     std::numeric_limits<float>::max() };
    slib::vec3 maxV{ -std::numeric_limits<float>::max(),
                     -std::numeric_limits<float>::max(),
                     -std::numeric_limits<float>::max() };
    bool hasGeometry = false;

    for (auto &solidPtr : scene.solids) {
      // Build model matrix (match ShadowRasterizer)
      slib::mat4 rotate = smath::rotation(
          slib::vec3({solidPtr->position.xAngle, solidPtr->position.yAngle, solidPtr->position.zAngle}));
      slib::mat4 translate = smath::translation(
          slib::vec3({solidPtr->position.x, solidPtr->position.y, solidPtr->position.z}));
      slib::mat4 scale = smath::scale(
          slib::vec3({solidPtr->position.zoom, solidPtr->position.zoom, solidPtr->position.zoom}));
      slib::mat4 modelMatrix = translate * rotate * scale;

      for (const auto &vData : solidPtr->vertexData) {
        slib::vec4 world = modelMatrix * slib::vec4(vData.vertex, 1);
        minV.x = std::min(minV.x, world.x);
        minV.y = std::min(minV.y, world.y);
        minV.z = std::min(minV.z, world.z);
        maxV.x = std::max(maxV.x, world.x);
        maxV.y = std::max(maxV.y, world.y);
        maxV.z = std::max(maxV.z, world.z);
        hasGeometry = true;
      }
    }

    // Fallback in case of no geometry
    slib::vec3 sceneCenter;
    float sceneRadius;
    if (hasGeometry) {
      sceneCenter = slib::vec3{ (minV.x + maxV.x) * 0.5f,
                                (minV.y + maxV.y) * 0.5f,
                                (minV.z + maxV.z) * 0.5f };
      slib::vec3 diag{ maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z };
      float diagLen2 = smath::dot(diag, diag);
      sceneRadius = 0.5f * std::sqrt(diagLen2);
      // Padding factor to avoid clipping at frustum edges
      //const float paddingFactor = 1.25f; // increase if needed
      //sceneRadius *= paddingFactor;
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
        shadowRasterizer.renderSolid(*solidPtr, *scene.shadowMap);
      }
    }
  }

  void prepareFrame(Scene &scene, float zNear, float zFar, float viewAngle) {

    // std::fill_n(scene.pixels, scene.screen.width * scene.screen.height, 0);
    std::copy(scene.backg,
              scene.backg + scene.screen.width * scene.screen.height,
              scene.pixels);
    scene.zBuffer->Clear(); // Clear the zBuffer

    zNear = 10.0f;    // Near plane distance
    zFar = 10000.0f;   // Far plane distance
    viewAngle = 45.0f; // Field of view angle in degrees

    float aspectRatio =
        (float)scene.screen.width / scene.screen.height; // Width / Height ratio
    float fovRadians = viewAngle * (PI / 180.0f);

    scene.projectionMatrix =
        smath::perspective(zFar, zNear, aspectRatio, fovRadians);

    if (scene.orbiting) {
      scene.viewMatrix =
          smath::lookAt(scene.camera.pos, scene.camera.orbitTarget, {0, 1, 0});
    } else {
      scene.viewMatrix = smath::fpsview(scene.camera.pos, scene.camera.pitch,
                                        scene.camera.yaw, scene.camera.roll);
    }

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
  void drawShadowMapOverlay(Scene &scene, int overlaySize = 200,
                            int margin = 10) {
    if (!scene.shadowMap || !scene.shadowsEnabled)
      return;

    const ShadowMap &sm = *scene.shadowMap;

    // Position in bottom-left corner
    int startX = margin;
    int startY = scene.screen.height - overlaySize - margin;

    // Find min/max depth for normalization (excluding max float values)
    float minDepth = std::numeric_limits<float>::max();
    float maxDepth = -std::numeric_limits<float>::max();

    for (int i = 0; i < sm.width * sm.height; ++i) {
      float d = sm.depthBuffer[i];
      if (d > -std::numeric_limits<float>::max() * 0.5f) {
        minDepth = std::min(minDepth, d);
        maxDepth = std::max(maxDepth, d);
      }
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

        float depth = sm.depthBuffer[smY * sm.width + smX];

        uint8_t gray;
        if (depth >= std::numeric_limits<float>::max() * 0.5f) {
          // No geometry - show as black
          gray = 0;
        } else {
          // Normalize depth to 0-255
          float normalized = (depth - minDepth) / depthRange;
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
  ShadowRasterizer shadowRasterizer;
};
