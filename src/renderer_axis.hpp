#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include "clipping.hpp"
#include "projection.hpp"
#include "scene.hpp"
#include "bresenham.hpp"
#include "effects/vertex_shaders.hpp"

class RendererAxis {
public:
  static void drawAxes(Scene &scene, float axisLength = 500.0f) {
    float gridSpacing = axisLength * 0.1f;
    drawGridPlanes(scene, axisLength, gridSpacing);
    drawAxisLabels(scene, axisLength);
  }

  static void drawGridPlanes(Scene &scene, float halfSize = 500.0f,
                             float spacing = 50.0f) {

    for (float t = -halfSize; t <= halfSize + 0.001f; t += spacing) {

      if (t == 0.0f) {
          drawAxisLine(scene, {-halfSize, 0, 0.0f}, {0, 0, 0.0f}, GREY_COLOR);
          drawAxisLine(scene, {0, 0, 0.0f}, {halfSize, 0, 0.0f}, RED_COLOR);

          drawAxisLine(scene, {0.0f, -halfSize, 0}, {0.0f, 0, 0}, GREY_COLOR);
          drawAxisLine(scene, {0.0f, 0, 0}, {0.0f, halfSize, 0}, GREEN_COLOR);
          
          drawAxisLine(scene, {0.0f, 0.0f, -halfSize}, {0.0f, 0.0f, 0}, GREY_COLOR);
          drawAxisLine(scene, {0.0f, 0.0f, 0}, {0.0f, 0.0f, halfSize}, BLUE_COLOR);
          continue;
      }

      drawAxisLine(scene, {-halfSize, t, 0.0f}, {halfSize, t, 0.0f}, GREY_COLOR);
      drawAxisLine(scene, {t, -halfSize, 0.0f}, {t, halfSize, 0.0f}, GREY_COLOR);

      drawAxisLine(scene, {-halfSize, 0.0f, t}, {halfSize, 0.0f, t}, GREY_COLOR);
      drawAxisLine(scene, {t, 0.0f, -halfSize}, {t, 0.0f, halfSize}, GREY_COLOR);
      
      drawAxisLine(scene, {0.0f, -halfSize, t}, {0.0f, halfSize, t}, GREY_COLOR);
      drawAxisLine(scene, {0.0f, t, -halfSize}, {0.0f, t, halfSize}, GREY_COLOR);
    }
  }

  static void drawAxisLabels(Scene &scene, float axisLength) {
    float labelOffset = axisLength * 0.12f;
    float labelSize = axisLength * 0.14f;

    drawLetterX(scene, {axisLength + labelOffset, 0.0f, 0.0f}, labelSize,
                {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, RED_COLOR);
    drawLetterY(scene, {0.0f, axisLength + labelOffset, 0.0f}, labelSize,
                {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, GREEN_COLOR);
    drawLetterZ(scene, {0.0f, 0.0f, axisLength + labelOffset}, labelSize,
                {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, BLUE_COLOR);
  }

private:
  using AxisVertex = vertex::Flat;
  
  static void drawAxisLine(Scene &scene, const slib::vec3 &start,
                           const slib::vec3 &end, uint32_t color) {
    AxisVertex v0;
    AxisVertex v1;
    v0.clip = slib::vec4(start, 1.0f) * scene.spaceMatrix;
    v1.clip = slib::vec4(end, 1.0f) * scene.spaceMatrix;

    if (!clipLineNdc(v0, v1)) {
      return;
    }

    if (!Projection<AxisVertex>::view(scene.screen.width, scene.screen.height,
                                      v0, true) ||
        !Projection<AxisVertex>::view(scene.screen.width, scene.screen.height,
                                      v1, true)) {
      return;
    }

    drawBresenhamLine(v0.p_x >> 16, v0.p_y >> 16, v0.p_z,
                      v1.p_x >> 16, v1.p_y >> 16, v1.p_z,
                      static_cast<uint32_t*>(scene.pixels), color,
                      scene.screen.width, scene.screen.height, scene.zBuffer.get());
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

};
