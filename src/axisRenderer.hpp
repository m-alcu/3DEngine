#pragma once

#include "clipping.hpp"
#include "projection.hpp"
#include "scene.hpp"
#include "bresenham.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

class AxisRenderer {
public:
  static void drawAxes(Scene &scene, float axisLength = 500.0f) {
    float gridSpacing = axisLength * 0.1f;
    drawGridPlanes(scene, axisLength, gridSpacing);
    drawAxisLabels(scene, axisLength);
  }

  static void drawGridPlanes(Scene &scene, float halfSize = 500.0f,
                             float spacing = 50.0f) {
    const uint32_t gridColor = 0xff404040;

    for (float t = -halfSize; t <= halfSize + 0.001f; t += spacing) {

      if (t == 0.0f) {
          drawAxisLine(scene, {-halfSize, 0, 0.0f}, {0, 0, 0.0f}, gridColor);
          drawAxisLine(scene, {0, 0, 0.0f}, {halfSize, 0, 0.0f}, 0xffff0000);

          drawAxisLine(scene, {0.0f, -halfSize, 0}, {0.0f, 0, 0}, gridColor);
          drawAxisLine(scene, {0.0f, 0, 0}, {0.0f, halfSize, 0}, 0xff00ff00);
          
          drawAxisLine(scene, {0.0f, 0.0f, -halfSize}, {0.0f, 0.0f, 0}, gridColor);
          drawAxisLine(scene, {0.0f, 0.0f, 0}, {0.0f, 0.0f, halfSize}, 0xff0000ff);
          continue;
      }

      drawAxisLine(scene, {-halfSize, t, 0.0f}, {halfSize, t, 0.0f}, gridColor);
      drawAxisLine(scene, {t, -halfSize, 0.0f}, {t, halfSize, 0.0f}, gridColor);

      drawAxisLine(scene, {-halfSize, 0.0f, t}, {halfSize, 0.0f, t}, gridColor);
      drawAxisLine(scene, {t, 0.0f, -halfSize}, {t, 0.0f, halfSize}, gridColor);

      drawAxisLine(scene, {0.0f, -halfSize, t}, {0.0f, halfSize, t}, gridColor);
      drawAxisLine(scene, {0.0f, t, -halfSize}, {0.0f, t, halfSize}, gridColor);
    }
  }

  static void drawAxisLabels(Scene &scene, float axisLength) {
    float labelOffset = axisLength * 0.12f;
    float labelSize = axisLength * 0.14f;

    drawLetterX(scene, {axisLength + labelOffset, 0.0f, 0.0f}, labelSize,
                {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0xffff0000);
    drawLetterY(scene, {0.0f, axisLength + labelOffset, 0.0f}, labelSize,
                {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, 0xff00ff00);
    drawLetterZ(scene, {0.0f, 0.0f, axisLength + labelOffset}, labelSize,
                {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xff0000ff);
  }

private:
  struct AxisVertex {
    slib::vec4 ndc;
    int32_t p_x = 0;
    int32_t p_y = 0;
    float p_z = 0.0f;
    bool broken = false;
  };

  static void drawAxisLine(Scene &scene, const slib::vec3 &start,
                           const slib::vec3 &end, uint32_t color) {
    AxisVertex v0;
    AxisVertex v1;
    v0.ndc = slib::vec4(start, 1.0f) * scene.spaceMatrix;
    v1.ndc = slib::vec4(end, 1.0f) * scene.spaceMatrix;

    if (!clipLineNdc(v0, v1)) {
      return;
    }

    if (!Projection<AxisVertex>::view(scene.screen.width, scene.screen.height,
                                      v0, true) ||
        !Projection<AxisVertex>::view(scene.screen.width, scene.screen.height,
                                      v1, true)) {
      return;
    }

    drawLineWithDepth(scene, v0, v1, color);
  }

  static void drawLetterX(Scene &scene, const slib::vec3 &center, float size,
                          const slib::vec3 &up, const slib::vec3 &right,
                          uint32_t color) {
    float half = size * 0.5f;
    drawAxisLine(scene, center - up * half - right * half,
                 center + up * half + right * half, color);
    drawAxisLine(scene, center - up * half + right * half,
                 center + up * half - right * half, color);
  }

  static void drawLetterY(Scene &scene, const slib::vec3 &center, float size,
                          const slib::vec3 &up, const slib::vec3 &right,
                          uint32_t color) {
    float half = size * 0.5f;
    float arm = size * 0.6f;
    slib::vec3 topLeft = center + up * half - right * (arm * 0.5f);
    slib::vec3 topRight = center + up * half + right * (arm * 0.5f);
    slib::vec3 junction = center + up * (size * 0.1f);
    slib::vec3 bottom = center - up * half;
    drawAxisLine(scene, topLeft, junction, color);
    drawAxisLine(scene, topRight, junction, color);
    drawAxisLine(scene, junction, bottom, color);
  }

  static void drawLetterZ(Scene &scene, const slib::vec3 &center, float size,
                          const slib::vec3 &up, const slib::vec3 &right,
                          uint32_t color) {
    float half = size * 0.5f;
    slib::vec3 topLeft = center + up * half - right * half;
    slib::vec3 topRight = center + up * half + right * half;
    slib::vec3 bottomLeft = center - up * half - right * half;
    slib::vec3 bottomRight = center - up * half + right * half;
    drawAxisLine(scene, topLeft, topRight, color);
    drawAxisLine(scene, topRight, bottomLeft, color);
    drawAxisLine(scene, bottomLeft, bottomRight, color);
  }

  static bool clipLineNdc(AxisVertex &a, AxisVertex &b) {
    for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right,
                            ClipPlane::Bottom, ClipPlane::Top,
                            ClipPlane::Near, ClipPlane::Far}) {
      bool aInside = IsInside(a, plane);
      bool bInside = IsInside(b, plane);

      if (aInside && bInside) {
        continue;
      }

      if (!aInside && !bInside) {
        return false;
      }

      if (aInside) {
        float alpha = ComputeAlpha(a, b, plane);
        b.ndc = a.ndc + (b.ndc - a.ndc) * alpha;
      } else {
        float alpha = ComputeAlpha(b, a, plane);
        a.ndc = b.ndc + (a.ndc - b.ndc) * alpha;
      }
    }

    return true;
  }

  static void drawLineWithDepth(Scene &scene, const AxisVertex &v0,
                                const AxisVertex &v1, uint32_t color) {
    int32_t x0 = v0.p_x;
    int32_t y0 = v0.p_y;
    int32_t x1 = v1.p_x;
    int32_t y1 = v1.p_y;
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;
    int steps = std::max(std::abs(dx >> 16), std::abs(dy >> 16));

    if (steps == 0) {
      return;
    }

    float invSteps = 1.0f / static_cast<float>(steps);
    int32_t xStep = dx * invSteps;
    int32_t yStep = dy * invSteps;
    float zStep = (v1.p_z - v0.p_z) * invSteps;

    int32_t x = x0;
    int32_t y = y0;
    float z = v0.p_z;

    uint32_t *pixels = static_cast<uint32_t *>(scene.pixels);

    for (int i = 0; i <= steps; ++i) {
      int xi = x >> 16;
      int yi = y >> 16;
      int pos = yi * scene.screen.width + xi;
      if (scene.zBuffer->TestAndSet(pos, z)) {
        pixels[pos] = color;
      }
      x += xStep;
      y += yStep;
      z += zStep;
    }
  }

};
