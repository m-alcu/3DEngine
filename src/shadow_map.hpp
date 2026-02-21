#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
#include "z_buffer.hpp"
#include "constants.hpp"
#include "light.hpp"
#include "scaler.hpp"
#include "bresenham.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "color.hpp"

class ShadowMap {
public:
  int numFaces;
  int faceWidth;
  int faceHeight;
  std::vector<std::unique_ptr<ZBuffer>> faces;
  std::vector<bool> faceDirty;
  std::vector<slib::mat4> lightSpaceMatrices;
  slib::mat4 lightProjMatrix;

  // Shadow bias parameters
  float maxSlopeBias = CUBE_SHADOW_MAX_SLOPE_BIAS;
  // PCF kernel size (0 = no filtering, 1 = 3x3, 2 = 5x5, etc.)
  int pcfRadius = SHADOW_PCF_RADIUS;

  // Cubemap depth range (used for 6-face point light shadows)
  float zNear = 0.1f;
  float zFar = 100.0f;

  ShadowMap(int w = 512, int h = 512, int nFaces = 1)
      : numFaces(nFaces), faceWidth(w), faceHeight(h),
        lightProjMatrix(smath::identity()) {
    faces.reserve(numFaces);
    for (int i = 0; i < numFaces; ++i) {
      faces.push_back(std::make_unique<ZBuffer>(w, h));
    }
    faceDirty.resize(numFaces, false);
    lightSpaceMatrices.resize(numFaces, smath::identity());
    clear();
  }

  void clear() {
    std::fill(faceDirty.begin(), faceDirty.end(), true);
  }

  void clearFaceIfDirty(int faceIdx) {
    if (faceDirty[faceIdx]) {
      faces[faceIdx]->Clear();
      faceDirty[faceIdx] = false;
    }
  }

  void resize(int w, int h) {
    faceWidth = w;
    faceHeight = h;
    for (auto& face : faces) {
      face->Resize(w, h);
    }
    clear();
  }

  bool isCubemap() const { return numFaces == 6; }
  int getFaceWidth() const { return faceWidth; }
  int getFaceHeight() const { return faceHeight; }

  const slib::mat4& getLightSpaceMatrix(int faceIdx = 0) const {
    return lightSpaceMatrices[faceIdx];
  }

  // Per-face depth access
  bool testAndSetDepth(int faceIdx, int pos, float depth) {
    return faces[faceIdx]->TestAndSet(pos, depth);
  }

  float getDepth(int faceIdx, int pos) const {
    return faces[faceIdx]->Get(pos);
  }

  // Convenience for face 0 (used by overlay)
  bool testAndSetDepth(int pos, float depth) {
    return faces[0]->TestAndSet(pos, depth);
  }

  float getDepth(int pos) const {
    return faces[0]->Get(pos);
  }

  // Draw single face as overlay on screen
  void drawFaceOverlay(int faceIdx, uint32_t* pixels, int screenW, int screenH,
                       int startX, int startY, int overlaySize) const {
    float minDepth = 1.0f;
    float maxDepth = -1.0f;
    for (int i = 0; i < faceWidth * faceHeight; ++i) {
      float d = faces[faceIdx]->Get(i);
      minDepth = std::min(minDepth, d);
      maxDepth = std::max(maxDepth, d);
    }
    float depthRange = std::max(maxDepth - minDepth, 0.0001f);

    int fw = faceWidth, fh = faceHeight;
    blitScaled(pixels, screenW, screenH,
               startX, startY, overlaySize, overlaySize,
               faceWidth, faceHeight,
               [&](int srcX, int srcY) -> uint32_t {
                 float depth = faces[faceIdx]->Get(srcY * fw + srcX);
                 uint8_t gray = (depth < 1.0f)
                     ? static_cast<uint8_t>(std::clamp((maxDepth - depth) / depthRange * 255.0f, 0.0f, 255.0f))
                     : 0;
                 return Color(gray, gray, gray).toBgra();
               });

    // Draw border
    int endX = startX + overlaySize - 1;
    int endY = startY + overlaySize - 1;
    drawBresenhamLine(startX, startY, endX, startY, pixels, WHITE_COLOR, screenW, screenH);
    drawBresenhamLine(startX, endY, endX, endY, pixels, WHITE_COLOR, screenW, screenH);
    drawBresenhamLine(startX, startY, startX, endY, pixels, WHITE_COLOR, screenW, screenH);
    drawBresenhamLine(endX, startY, endX, endY, pixels, WHITE_COLOR, screenW, screenH);
  }

  // Legacy single-face overlay (delegates to face 0)
  void drawOverlay(uint32_t* pixels, int screenW, int screenH,
                   int startX, int startY, int overlaySize) const {
    drawFaceOverlay(0, pixels, screenW, screenH, startX, startY, overlaySize);
  }

  // Build light-space matrices for shadow mapping
  void buildLightMatrices(const Light &light, const slib::vec3 &sceneCenter,
                          float sceneRadius) {
    if (light.type == LightType::Directional) {
      buildDirectionalLightMatrices(light, sceneCenter, sceneRadius);
    } else if (light.type == LightType::Point) {
      if (numFaces == 6) {
        buildCubemapMatrices(light, sceneRadius);
        return; // lightSpaceMatrices already set for all 6 faces
      } else {
        buildPointLightMatrices(light, sceneCenter, sceneRadius);
      }
    } else if (light.type == LightType::Spot) {
      buildSpotLightMatrices(light, sceneRadius);
    }


  }

  // Sample shadow at a world position
  // Returns: 1.0 = fully lit, 0.0 = fully shadowed
  float sampleShadow(const slib::vec3 &worldPos, float cosTheta,
                    const slib::vec3 &lightPos = {0, 0, 0}) const {
    if (numFaces == 6) {
      int faceIdx = selectFace(worldPos - lightPos);
      if (faceDirty[faceIdx]) return 1.0f;
      return sampleFace(faceIdx, worldPos, cosTheta);
    }
    if (faceDirty[0]) return 1.0f;
    return sampleFace(0, worldPos, cosTheta);
  }

private:
  // Temporary storage for single-face light matrix building
  slib::mat4 lightViewMatrix = smath::identity();

  // Select cubemap face based on direction from light to point
  int selectFace(const slib::vec3& dir) const {
    float absX = std::abs(dir.x);
    float absY = std::abs(dir.y);
    float absZ = std::abs(dir.z);

    if (absX >= absY && absX >= absZ) {
      return dir.x > 0 ? 0 : 1;  // +X or -X
    } else if (absY >= absX && absY >= absZ) {
      return dir.y > 0 ? 2 : 3;  // +Y or -Y
    } else {
      return dir.z > 0 ? 4 : 5;  // +Z or -Z
    }
  }

  // Sample a single face at a world position
  float sampleFace(int faceIdx, const slib::vec3& worldPos, float cosTheta) const {
    slib::vec4 lightSpacePos = slib::vec4(worldPos, 1.0f) * lightSpaceMatrices[faceIdx];

    if (std::abs(lightSpacePos.w) < 0.0001f) {
      return 1.0f;
    }

    float oneOverW = 1.0f / lightSpacePos.w;
    float ndcX = lightSpacePos.x * oneOverW;
    float ndcY = lightSpacePos.y * oneOverW;
    float currentDepth = lightSpacePos.z * oneOverW;

    int sx = static_cast<int>((ndcX * 0.5f + 0.5f) * faceWidth + 0.5f);
    int sy = static_cast<int>((-ndcY * 0.5f + 0.5f) * faceHeight + 0.5f);

    // Behind the light or beyond range = lit
    if (currentDepth < -1.0f) {
      return 1.0f;
    }
    if (numFaces == 6 && (currentDepth > 1.0f || lightSpacePos.w < 0.0f)) {
      return 1.0f;
    }

    // Slope-scaled bias in NDC space
    float texelDepth = 2.0f / faceWidth;
    cosTheta = std::clamp(cosTheta, 0.0f, 1.0f);
    float slopeFactor = (cosTheta > 0.01f)
        ? std::min(1.0f / cosTheta, maxSlopeBias)
        : maxSlopeBias;
    float bias = texelDepth * slopeFactor;

    if (pcfRadius < 1) {
      return sampleShadowSingle(faceIdx, sx, sy, currentDepth, bias);
    } else {
      return sampleShadowPCF(faceIdx, sx, sy, currentDepth, bias);
    }
  }

  inline float sampleShadowSingle(int faceIdx, int sx, int sy,
                                  float currentDepth, float bias) const {
    if (sx < 0 || sx >= faceWidth || sy < 0 || sy >= faceHeight) {
      return 1.0f;
    }

    float storedDepth = faces[faceIdx]->Get(sy * faceWidth + sx);
    return (currentDepth - bias < storedDepth) ? 1.0f : 0.0f;
  }

  // PCF (Percentage Closer Filtering) for soft shadow edges
  float sampleShadowPCF(int faceIdx, int sx, int sy,
                        float currentDepth, float bias) const {
    int shadow = 0;
    int samples = 0;

    for (int dy = -pcfRadius; dy <= pcfRadius; dy++) {
      int dsy = sy + dy;
      if (dsy < 0 || dsy >= faceHeight)
        continue;

      for (int dx = -pcfRadius; dx <= pcfRadius; dx++) {
        int dsx = sx + dx;
        if (dsx < 0 || dsx >= faceWidth)
          continue;

        float storedDepth = faces[faceIdx]->Get(dsy * faceWidth + dsx);
        shadow += (currentDepth - bias < storedDepth) ? 1 : 0;
        samples++;
      }
    }

    if (shadow == 0) {
      return 0.0f;
    } else if (shadow == samples) {
      return 1.0f;
    } else {
      return static_cast<float>(shadow) / samples;
    }
  }

  void buildDirectionalLightMatrices(const Light &light,
                                     const slib::vec3 &sceneCenter,
                                     float sceneRadius) {
    slib::vec3 lightDir = smath::normalize(light.direction);
    slib::vec3 lightPos = sceneCenter + lightDir * sceneRadius * 2.0f;

    slib::vec3 up = {0.0f, 1.0f, 0.0f};
    if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
      up = {1.0f, 0.0f, 0.0f};
    }

    lightViewMatrix = smath::lookAt(lightPos, sceneCenter, up);

    float size = sceneRadius * 1.2f;
    lightProjMatrix =
        smath::ortho(-size, size, -size, size, 0.1f, sceneRadius * 4.0f);
  }

  void buildPointLightMatrices(const Light &light,
                               const slib::vec3 &sceneCenter,
                               float sceneRadius) {
    slib::vec3 up = {0.0f, 1.0f, 0.0f};
    slib::vec3 lightDir = smath::normalize(sceneCenter - light.position);
    if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
      up = {1.0f, 0.0f, 0.0f};
    }

    lightViewMatrix = smath::lookAt(light.position, sceneCenter, up);

    slib::vec3 toScene = sceneCenter - light.position;
    float distToScene = std::sqrt(smath::dot(toScene, toScene));
    distToScene = std::max(distToScene, 1.0f);
    float effectiveRadius = sceneRadius * EFFECTIVE_LIGHT_RADIUS_FACTOR;
    float fov = 2.0f * std::atan(effectiveRadius / distToScene);
    fov = std::clamp(fov, 20.0f * RAD, 90.0f * RAD);

    float aspect = static_cast<float>(faceWidth) / faceHeight;

    float _zNear, _zFar;
    if (distToScene > sceneRadius * 1.5f) {
      _zNear = std::max(1.0f, distToScene - sceneRadius);
      _zFar = distToScene + sceneRadius * 2.0f;
    } else {
      _zNear = std::max(0.1f, distToScene * 0.05f);
      _zFar = std::max(_zNear * 2.0f, distToScene + sceneRadius * 1.2f);
    }

    const float maxDepthRatio = 300.0f;
    if (_zFar / _zNear > maxDepthRatio) {
      _zNear = _zFar / maxDepthRatio;
    }

    lightProjMatrix = smath::perspective(_zFar, _zNear, aspect, fov);
    
    // For single-face, store combined matrix in slot 0
    lightSpaceMatrices[0] = lightViewMatrix * lightProjMatrix;
  }

  void buildCubemapMatrices(const Light &light, float sceneRadius) {
    zNear = std::max(0.1f, sceneRadius * 0.01f);
    zFar = std::max(light.radius * 2.0f, sceneRadius * 3.0f);

    float aspect = 1.0f;
    float fov = PI / 2.0f;
    lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);

    const slib::vec3& pos = light.position;

    // +X, -X, +Y, -Y, +Z, -Z
    slib::mat4 views[6] = {
      smath::lookAt(pos, pos + slib::vec3{1, 0, 0}, {0, -1, 0}),
      smath::lookAt(pos, pos + slib::vec3{-1, 0, 0}, {0, -1, 0}),
      smath::lookAt(pos, pos + slib::vec3{0, 1, 0}, {0, 0, 1}),
      smath::lookAt(pos, pos + slib::vec3{0, -1, 0}, {0, 0, -1}),
      smath::lookAt(pos, pos + slib::vec3{0, 0, 1}, {0, -1, 0}),
      smath::lookAt(pos, pos + slib::vec3{0, 0, -1}, {0, -1, 0}),
    };

    for (int i = 0; i < 6; ++i) {
      lightSpaceMatrices[i] = views[i] * lightProjMatrix;
    }
  }

  void buildSpotLightMatrices(const Light &light, float sceneRadius) {
    slib::vec3 lightDir = smath::normalize(light.direction);
    slib::vec3 target = light.position + lightDir * light.radius;

    slib::vec3 up = {0.0f, 1.0f, 0.0f};
    if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
      up = {1.0f, 0.0f, 0.0f};
    }

    lightViewMatrix = smath::lookAt(light.position, target, up);

    float fov = std::acos(light.outerCutoff) * 2.0f;
    float aspect = static_cast<float>(faceWidth) / faceHeight;
    float _zNear = 1.0f;
    float _zFar = light.radius * 2.0f;

    lightProjMatrix = smath::perspective(_zFar, _zNear, aspect, fov);
  }
};
