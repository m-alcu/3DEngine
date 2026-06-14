#pragma once

#include <algorithm>
#include "bresenham.hpp"
#include "constants.hpp"
#include "renderer_fonts.hpp"
#include "scaler.hpp"
#include "scene.hpp"
#include "shadow_map.hpp"
#include "slib.hpp"

namespace RendererOverlay {

// Draw a single shadow-map face as a grayscale depth overlay with a border.
inline void drawShadowFaceOverlay(const ShadowMap &shadowMap, int faceIdx,
                                  uint32_t *pixels, int screenW, int screenH,
                                  int startX, int startY, int overlaySize) {
  int fw = shadowMap.getFaceWidth();
  int fh = shadowMap.getFaceHeight();

  float minDepth = 1.0f;
  float maxDepth = -1.0f;
  for (int i = 0; i < fw * fh; ++i) {
    float d = shadowMap.getDepth(faceIdx, i);
    minDepth = std::min(minDepth, d);
    maxDepth = std::max(maxDepth, d);
  }
  float depthRange = std::max(maxDepth - minDepth, 0.0001f);

  blitScaled(pixels, screenW, screenH, startX, startY, overlaySize, overlaySize,
             fw, fh, [&](int srcX, int srcY) -> uint32_t {
               float depth = shadowMap.getDepth(faceIdx, srcY * fw + srcX);
               uint8_t gray = (depth < 1.0f)
                   ? static_cast<uint8_t>(std::clamp(
                         (maxDepth - depth) / depthRange * 255.0f, 0.0f, 255.0f))
                   : 0;
               return slib::vec3(gray, gray, gray).toBgra();
             });

  int endX = startX + overlaySize - 1;
  int endY = startY + overlaySize - 1;
  drawBresenhamLine(startX, startY, endX, startY, pixels, WHITE_COLOR, screenW, screenH);
  drawBresenhamLine(startX, endY, endX, endY, pixels, WHITE_COLOR, screenW, screenH);
  drawBresenhamLine(startX, startY, startX, endY, pixels, WHITE_COLOR, screenW, screenH);
  drawBresenhamLine(endX, startY, endX, endY, pixels, WHITE_COLOR, screenW, screenH);
}

inline ShadowMap *findShadowMapForOverlay(Scene &scene) {
  // Check selected entity first
  if (scene.selectedEntityIndex >= 0 &&
      scene.selectedEntityIndex < static_cast<int>(scene.entities.size())) {
    Entity selectedEntity = scene.entities[scene.selectedEntityIndex];
    auto *shadowComponent = scene.registry.shadows().get(selectedEntity);
    if (shadowComponent && shadowComponent->shadowMap) {
      return shadowComponent->shadowMap.get();
    }
  }

  // Fall back to first light source entity's shadow map
  for (const auto &[entity, shadowComponent] : scene.registry.shadows()) {
    if (shadowComponent.shadowMap) {
      return shadowComponent.shadowMap.get();
    }
  }

  return nullptr;
}

// Draw shadow map overlay - handles both single-face and cubemap
inline void drawShadowMapOverlay(Scene &scene, int margin = 10) {
  if (!scene.shadowsEnabled)
    return;

  ShadowMap *shadowMapPtr = findShadowMapForOverlay(scene);
  if (!shadowMapPtr)
    return;

  if (shadowMapPtr->isCubemap()) {
    // Draw 6 cubemap faces in a row
    int faceOverlaySize = SHADOW_MAP_OVERVIEW_SIZE / 2;
    static const char *faceLabels[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
    int startY = scene.screen.height - faceOverlaySize - margin;

    for (int faceIdx = 0; faceIdx < 6; ++faceIdx) {
      int startX = margin + faceIdx * (faceOverlaySize + 2);
      drawShadowFaceOverlay(*shadowMapPtr, faceIdx, scene.pixels.data(),
                            scene.screen.width, scene.screen.height, startX,
                            startY, faceOverlaySize);
      RendererFonts::drawText(scene.pixels.data(), scene.screen.width, scene.screen.height,
                              scene.screen.width, startX + 2, startY + 2,
                              faceLabels[faceIdx], WHITE_COLOR, BLACK_COLOR, true,
                              scene.font);
    }
  } else {
    // Single face overlay
    int overlaySize = SHADOW_MAP_OVERVIEW_SIZE;
    int startX = margin;
    int startY = scene.screen.height - overlaySize - margin;
    drawShadowFaceOverlay(*shadowMapPtr, 0, scene.pixels.data(), scene.screen.width,
                          scene.screen.height, startX, startY, overlaySize);
  }
}

} // namespace RendererOverlay
