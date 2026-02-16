#pragma once

#include "scene.hpp"
#include "vendor/imgui/imgui.h"

namespace SceneUI {

inline void drawSolidControls(Scene& scene) {
    if (scene.entities.empty()) return;

    scene.selectedEntityIndex = std::clamp(scene.selectedEntityIndex, 0,
                                     static_cast<int>(scene.entities.size() - 1));

    // Build solid labels for combo
    std::vector<std::string> solidLabels;
    solidLabels.reserve(scene.entities.size());
    std::vector<const char*> solidLabelPtrs;
    solidLabelPtrs.reserve(scene.entities.size());

    for (size_t i = 0; i < scene.entities.size(); ++i) {
      Entity entity = scene.entities[i];
      const auto* nameComp = scene.registry.names().get(entity);
      const std::string& solidName = nameComp ? nameComp->name : std::string();
      if (!solidName.empty()) {
        solidLabels.push_back(solidName);
      } else {
        solidLabels.push_back("Solid " + std::to_string(i));
      }
      solidLabelPtrs.push_back(solidLabels.back().c_str());
    }

    if (ImGui::Combo("Selected Solid", &scene.selectedEntityIndex,
                     solidLabelPtrs.data(),
                     static_cast<int>(solidLabelPtrs.size()))) {
      Entity entity = scene.entities[scene.selectedEntityIndex];
      auto* transform = scene.registry.transforms().get(entity);
      auto* mesh = scene.registry.meshes().get(entity);
      if (transform && mesh) {
        scene.camera.orbitTarget = TransformSystem::getWorldCenter(*transform);
        scene.camera.setOrbitFromCurrent();
      }
    }

    Entity entity = scene.entities[scene.selectedEntityIndex];
    auto* render = scene.registry.renders().get(entity);
    auto* transform = scene.registry.transforms().get(entity);
    if (!render || !transform) {
      return;
    }

    int currentShading = static_cast<int>(render->shading);
    if (ImGui::Combo("Shading", &currentShading, shadingNames,
                     IM_ARRAYSIZE(shadingNames))) {
      render->shading = static_cast<Shading>(currentShading);
    }

    auto* rotation = scene.registry.rotations().get(entity);
    if (rotation) {
      ImGui::Checkbox("Rotate", &rotation->enabled);
      ImGui::SliderFloat("Rot X Speed", &rotation->incXangle, 0.0f, 1.0f);
      ImGui::SliderFloat("Rot Y Speed", &rotation->incYangle, 0.0f, 1.0f);
    }

    float position[3] = {transform->position.x,
                         transform->position.y,
                         transform->position.z};
    if (ImGui::DragFloat3("Position", position, 1.0f)) {
      transform->position.x = position[0];
      transform->position.y = position[1];
      transform->position.z = position[2];
    }

    ImGui::DragFloat("Zoom", &transform->position.zoom, 0.1f, 0.01f, 500.0f);

    float angles[3] = {transform->position.xAngle,
                       transform->position.yAngle,
                       transform->position.zAngle};
    if (ImGui::DragFloat3("Angles", angles, 1.0f, -360.0f, 360.0f)) {
      transform->position.xAngle = angles[0];
      transform->position.yAngle = angles[1];
      transform->position.zAngle = angles[2];
    }

    bool orbitEnabled = transform->orbit.enabled;
    if (ImGui::Checkbox("Enable Orbit", &orbitEnabled)) {
      if (orbitEnabled) {
        TransformSystem::enableCircularOrbit(*transform,
                                             transform->orbit.center,
                                             transform->orbit.radius,
                                             transform->orbit.n,
                                             transform->orbit.omega,
                                             transform->orbit.phase);
      } else {
        TransformSystem::disableCircularOrbit(*transform);
      }
    }

    float orbitCenter[3] = {transform->orbit.center.x,
                            transform->orbit.center.y,
                            transform->orbit.center.z};
    if (ImGui::DragFloat3("Orbit Center", orbitCenter, 1.0f)) {
      transform->orbit.center = {orbitCenter[0], orbitCenter[1], orbitCenter[2]};
    }

    ImGui::DragFloat("Orbit Radius", &transform->orbit.radius, 0.1f, 0.0f, 10000.0f);
    ImGui::DragFloat("Orbit Speed", &transform->orbit.omega, 0.01f, -10.0f, 10.0f);

    // Light properties (only shown if solid is a light source)
    auto* lightComponent = scene.registry.lights().get(entity);
    if (lightComponent) {
      ImGui::Separator();
      ImGui::Text("Light Source");
      ImGui::SliderFloat("Light Intensity", &lightComponent->light.intensity, 0.0f, 100.0f);
    }
}

inline void drawSceneControls(Scene& scene) {
    int currentBackground = static_cast<int>(scene.backgroundType);
    if (ImGui::Combo("Background", &currentBackground, backgroundNames,
                     IM_ARRAYSIZE(backgroundNames))) {
      scene.backgroundType = static_cast<BackgroundType>(currentBackground);
      scene.background = std::unique_ptr<Background>(
          BackgroundFactory::createBackground(scene.backgroundType));
    }

    ImGui::Checkbox("Show Axis Helper", &scene.showAxes);
    ImGui::Checkbox("Face Depth Sorting", &scene.depthSortEnabled);
    ImGui::Checkbox("Shadows Enabled", &scene.shadowsEnabled);
    ImGui::Checkbox("Show Shadow Map Overlay", &scene.showShadowMapOverlay);
    ImGui::Checkbox("Use Cubemap Shadows (Point Lights)", &scene.useCubemapShadows);

    static const char* pcfLabels[] = {"Off (0)", "3x3 (1)", "5x5 (2)"};
    int currentPcfRadius = scene.pcfRadius;
    if (ImGui::Combo("PCF Radius", &currentPcfRadius, pcfLabels, IM_ARRAYSIZE(pcfLabels))) {
      scene.pcfRadius = currentPcfRadius;
    }

    ImGui::Separator();
    ImGui::Text("Shadow Bias Configuration");
    ImGui::SliderFloat("Min Bias", &scene.minBiasDefault, 0.001f, 1.0f, "%.4f");
    ImGui::SliderFloat("Max Bias", &scene.maxBiasDefault, 0.001f, 1.0f, "%.4f");
    ImGui::SliderFloat("Min Shadow Bias", &scene.shadowBiasMin, 0.001f, 1.0f, "%.4f");
    ImGui::SliderFloat("Max Shadow Bias", &scene.shadowBiasMax, 0.01f, 1.0f, "%.4f");
    ImGui::SliderFloat("Cube Max Slope Bias", &scene.cubeShadowMaxSlopeBias, 1.0f, 50.0f, "%.1f");

    ImGui::Separator();
    static const char* fontLabels[] = {"Default", "IBM CGA", "ZX Spectrum", "Amstrad CPC", "Commodore 64", "Atari 8-bit", "Retro"};
    int currentFont = static_cast<int>(scene.font);
    if (ImGui::Combo("Font", &currentFont, fontLabels, IM_ARRAYSIZE(fontLabels))) {
      scene.font = static_cast<Font8x8::FontType>(currentFont);
    }

    ImGui::Separator();
    ImGui::Text("Scene Center: (%.2f, %.2f, %.2f)", scene.sceneCenter.x, scene.sceneCenter.y, scene.sceneCenter.z);
    ImGui::Text("Scene Radius: %.2f", scene.sceneRadius);
}

inline void drawCameraInfo(const Scene& scene) {
    ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", scene.camera.pos.x, scene.camera.pos.y, scene.camera.pos.z);
    ImGui::Text("Camera for: (%.2f, %.2f, %.2f)", scene.camera.forward.x, scene.camera.forward.y, scene.camera.forward.z);
    ImGui::Text("OrbitTarget: (%.2f, %.2f, %.2f)", scene.camera.orbitTarget.x, scene.camera.orbitTarget.y, scene.camera.orbitTarget.z);
    ImGui::Text("Camera Pitch: %.2f, Yaw: %.2f, Roll: %.2f", scene.camera.pitch, scene.camera.yaw, scene.camera.roll);
}

inline void drawStats(const Scene& scene) {
    ImGui::Separator();
    ImGui::Text("Polys rendered: %u", scene.stats.polysRendered);
    ImGui::Text("Pixels rasterized: %u", scene.stats.pixelsRasterized);
    ImGui::Text("Draw calls: %u", scene.stats.drawCalls);
    ImGui::Text("Vertices processed: %u", scene.stats.verticesProcessed);
}

} // namespace SceneUI
