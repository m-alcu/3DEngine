#pragma once

#include "constants.hpp"
#include "fonts.hpp"
#include "scene.hpp"
#include "shadow_map.hpp"

namespace RendererOverlay {

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
      shadowMapPtr->drawFaceOverlay(faceIdx, scene.pixels, scene.screen.width,
                                    scene.screen.height, startX, startY,
                                    faceOverlaySize);
      uint32_t labelColor =
          shadowMapPtr->faceDirty[faceIdx] ? RED_COLOR : WHITE_COLOR;
      Font8x8::drawText(scene.pixels, scene.screen.width, scene.screen.height,
                        scene.screen.width, startX + 2, startY + 2,
                        faceLabels[faceIdx], labelColor, BLACK_COLOR, true,
                        scene.font);
    }
  } else {
    // Single face overlay
    int overlaySize = SHADOW_MAP_OVERVIEW_SIZE;
    int startX = margin;
    int startY = scene.screen.height - overlaySize - margin;
    shadowMapPtr->drawOverlay(scene.pixels, scene.screen.width,
                              scene.screen.height, startX, startY, overlaySize);
  }
}

} // namespace RendererOverlay
