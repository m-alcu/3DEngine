#pragma once

#include "ZBuffer.hpp"
#include "constants.hpp"
#include "events/EventManager.hpp"
#include "light.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include <algorithm>
#include <cmath>

class ShadowMap {
public:
  int width;
  int height;
  ZBuffer zbuffer;
  slib::mat4 lightViewMatrix;
  slib::mat4 lightProjMatrix;

  // Combined matrix for transforming world coords to light clip space
  slib::mat4 lightSpaceMatrix;

  // Shadow bias parameters for slope-scaled bias
  float minBias = 0.025f; // Minimum bias (surfaces facing the light)
  float maxBias = 0.05f;  // Maximum bias (surfaces at grazing angles)

  // PCF kernel size (0 = no filtering, 1 = 3x3, 2 = 5x5, etc.)
  int pcfRadius = SHADOW_PCF_RADIUS;

  // Event manager to keep callback subscriptions alive
  sage::EventManager eventManager;

  ShadowMap(int w = 512, int h = 512)
      : width(w), height(h), zbuffer(w, h), lightViewMatrix(smath::identity()),
        lightProjMatrix(smath::identity()),
        lightSpaceMatrix(smath::identity()) {
    zbuffer.Clear();
  }

  void clear() { zbuffer.Clear(); }

  // Subscribe to pcfRadius changes from a source (e.g., Scene)
  void subscribeToPcfRadiusChanges(sage::Event& event, int& sourceRadius) {
    eventManager.Subscribe([this, &sourceRadius]() {
      this->pcfRadius = sourceRadius;
    }, event);
  }

  void resize(int w, int h) {
    width = w;
    height = h;
    zbuffer.Resize(w, h);
    clear();
  }

  // Test and set depth (like z-buffer) - returns true if depth should be
  // written
  bool testAndSetDepth(int pos, float depth) {
    return zbuffer.TestAndSet(pos, depth);
  }

  // Get depth at pixel (x, y)
  float getDepth(int x, int y) const {
    size_t idx = static_cast<size_t>(y) * width + x;
    return zbuffer.Get(static_cast<int>(idx));
  }

  // Build light-space matrices for shadow mapping
  // sceneCenter: approximate center of the scene being shadowed
  // sceneRadius: approximate radius encompassing shadow casters/receivers
  void buildLightMatrices(const Light &light, const slib::vec3 &sceneCenter,
                          float sceneRadius) {
    if (light.type == LightType::Directional) {
      buildDirectionalLightMatrices(light, sceneCenter, sceneRadius);
    } else if (light.type == LightType::Point) {
      buildPointLightMatrices(light, sceneCenter, sceneRadius);
    } else if (light.type == LightType::Spot) {
      buildSpotLightMatrices(light, sceneRadius);
    }

    // Pre-compute combined matrix
    lightSpaceMatrix = lightViewMatrix * lightProjMatrix;

    // Auto-calculate bias based on scene parameters
    calculateMinMaxBias(sceneRadius);
  }

  // Sample shadow at a world position and cosTheta in case is available
  // Returns: 1.0 = fully lit, 0.0 = fully shadowed
  float sampleShadow(const slib::vec3 &worldPos, float cosTheta) const {
    float dynamicBias = calculateBias(cosTheta);
    // Transform world position to light clip space
    slib::vec4 lightSpacePos = slib::vec4(worldPos, 1.0f) * lightSpaceMatrix;

    // Perspective divide (for point/spot lights)
    if (std::abs(lightSpacePos.w) < 0.0001f) {
      return 1.0f; // Avoid division by zero
    }

    float oneOverW = 1.0f / lightSpacePos.w;

    float ndcX = lightSpacePos.x * oneOverW;
    float ndcY = lightSpacePos.y * oneOverW;
    float currentDepth = lightSpacePos.z * oneOverW;

    // Map from NDC [-1,1] to texture coords [0, screen]
    int sx = static_cast<int>((ndcX * 0.5f + 0.5f) * width + 0.5f); // Convert from NDC to screen coordinates
    int sy = static_cast<int>((-ndcY * 0.5f + 0.5f) * height + 0.5f); // Convert from NDC to screen coordinates

    // Behind the light = lit
    if (currentDepth < -1.0f) {
      return 1.0f;
    }

    if (pcfRadius < 1) {
      return sampleShadowSingle(sx, sy, currentDepth, dynamicBias);
    } else {
      return sampleShadowPCF(sx, sy, currentDepth, dynamicBias);
    }
  }

private:
  // Calculate optimal bias values based on shadow map resolution and scene
  // scale Depth is stored in NDC space [-1, 1] after perspective divide (z/w)
  void calculateMinMaxBias(float sceneRadius) {
    // Since depth is in NDC [-1, 1], the total depth range is 2.0
    // The bias needs to account for:
    // 1. Shadow map resolution (lower res = larger texels = more error)
    // 2. Depth quantization in NDC space

    // NDC depth range is always 2.0 (from -1 to 1)
    constexpr float ndcDepthRange = 2.0f;

    // A single texel spans this much in UV space: 1/width
    // The depth error per texel is roughly: ndcDepthRange / width
    // This represents the minimum depth difference we can reliably detect
    float depthPerTexel = ndcDepthRange / static_cast<float>(width);

    // minBias: for surfaces perpendicular to light (cosTheta ≈ 1)
    // Need enough bias to overcome depth precision issues (~1 texel)
    minBias = depthPerTexel * 0.5f;

    // maxBias: for surfaces at grazing angles (cosTheta ≈ 0)
    // Slope causes projected depth error, need more bias (~2-4 texels)
    maxBias = depthPerTexel * 2.0f;

    // Clamp to reasonable NDC values
    minBias = std::clamp(minBias, SHADOW_BIAS_MIN, 0.025f);
    maxBias = std::clamp(maxBias, minBias * 2.0f, SHADOW_BIAS_MAX);
  }

  // Calculate dynamic slope-scaled bias based on surface angle to light
  // normal: surface normal (normalized)
  // lightDir: direction TO the light (normalized)
  float calculateBias(float cosTheta) const {
    // Slope scale: when surface is perpendicular to light (cosTheta=1), use
    // minBias When surface is at grazing angle (cosTheta~0), use maxBias Using
    // tan(acos(x)) = sqrt(1-x²)/x for slope factor
    float slopeFactor = 1.0f;
    if (cosTheta > 0.001f) {
      slopeFactor = std::sqrt(1.0f - cosTheta * cosTheta) / cosTheta;
      slopeFactor =
          std::min(slopeFactor, 10.0f); // Clamp to avoid extreme values
    }

    return std::clamp(minBias + minBias * slopeFactor, minBias, maxBias);
  }

  // Single sample shadow test
  // Note: u, v assumed to be in [0, 1] range (caller validates)
  inline float sampleShadowSingle(int sx, int sy, float currentDepth,
                                  float bias) const {

    // Out of shadow map bounds = lit (no shadow info)
    if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
      return 1.0f;
    }

    float storedDepth = getDepth(sx, sy);
    // If current depth (minus bias) is less than stored depth, we're in shadow
    return (currentDepth - bias < storedDepth) ? 1.0f : 0.0f;
  }

  // PCF (Percentage Closer Filtering) for soft shadow edges
  // More details in: https://developer.nvidia.com/gpugems/gpugems/part-ii-lighting-and-shadows/chapter-11-shadow-map-antialiasing
  float sampleShadowPCF(int sx, int sy, float currentDepth,
                        float bias) const {
    int shadow = 0;
    int samples = 0;

    for (int dy = -pcfRadius; dy <= pcfRadius; dy++) {
      int dsy = sy + dy;
      if (dsy < 0 || dsy >= height)
        continue;

      for (int dx = -pcfRadius; dx <= pcfRadius; dx++) {
        int dsx = sx + dx;
        if (dsx < 0 || dsx >= width)
          continue;

        float storedDepth = getDepth(dsx, dsy);
        shadow += (currentDepth - bias < storedDepth) ? 1 : 0;
        samples++;
      }
    }

    if (shadow == 0) {
      return 0.0f; // Fully shadowed
    } else if (shadow == samples) {
      return 1.0f; // Fully lit
    } else { 
      return static_cast<float>(shadow) / samples;
    }  
  }

  void buildDirectionalLightMatrices(const Light &light,
                                     const slib::vec3 &sceneCenter,
                                     float sceneRadius) {
    // Position the "light camera" far enough back along the light direction
    // Note: light.direction points FROM the light source (inverted convention),
    // so we ADD to move back toward where the light originates
    slib::vec3 lightDir = smath::normalize(light.direction);
    slib::vec3 lightPos = sceneCenter + lightDir * sceneRadius * 2.0f;

    // Choose an up vector that isn't parallel to light direction
    slib::vec3 up = {0.0f, 1.0f, 0.0f};
    if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
      up = {1.0f, 0.0f, 0.0f};
    }

    lightViewMatrix = smath::lookAt(lightPos, sceneCenter, up);

    // Orthographic projection sized to encompass the scene
    float size = sceneRadius * 1.2f;
    lightProjMatrix =
        smath::ortho(-size, size, -size, size, 0.1f, sceneRadius * 4.0f);
  }

  void buildPointLightMatrices(const Light &light,
                               const slib::vec3 &sceneCenter,
                               float sceneRadius) {
    // For point lights, we look from the light position toward the scene center
    // Note: Full point light shadows would need 6 faces (cubemap), this is
    // simplified

    slib::vec3 up = {0.0f, 1.0f, 0.0f};
    slib::vec3 lightDir = smath::normalize(sceneCenter - light.position);
    if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
      up = {1.0f, 0.0f, 0.0f};
    }

    lightViewMatrix = smath::lookAt(light.position, sceneCenter, up);

    // Calculate FOV dynamically to encompass the scene
    slib::vec3 toScene = sceneCenter - light.position;
    float distToScene = std::sqrt(smath::dot(toScene, toScene));
    // Avoid division by zero and ensure minimum distance
    distToScene = std::max(distToScene, 1.0f);
    // Use smaller effective radius to make objects fill more of the shadow map
    float effectiveRadius = sceneRadius * EFFECTIVE_LIGHT_RADIUS_FACTOR;
    float fov = 2.0f * std::atan(effectiveRadius / distToScene);
    // Clamp to reasonable range
    fov = std::clamp(fov, 20.0f * (PI / 180.0f), 90.0f * (PI / 180.0f));

    float aspect = static_cast<float>(width) / height;
    float zNear = std::max(1.0f, distToScene - sceneRadius);
    float zFar = distToScene + sceneRadius * 2.0f;

    lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);
  }

  void buildSpotLightMatrices(const Light &light, float sceneRadius) {
    // Spot light looks along its direction
    slib::vec3 lightDir = smath::normalize(light.direction);
    slib::vec3 target = light.position + lightDir * light.radius;

    slib::vec3 up = {0.0f, 1.0f, 0.0f};
    if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
      up = {1.0f, 0.0f, 0.0f};
    }

    lightViewMatrix = smath::lookAt(light.position, target, up);

    // FOV based on outer cutoff angle
    float fov = std::acos(light.outerCutoff) * 2.0f;
    float aspect = static_cast<float>(width) / height;
    float zNear = 1.0f;
    float zFar = light.radius * 2.0f;

    lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);
  }
};
