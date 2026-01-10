#pragma once

#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>
#include "slib.hpp"
#include "smath.hpp"
#include "light.hpp"

class ShadowMap {
public:
    int width;
    int height;
    std::vector<float> depthBuffer;
    slib::mat4 lightViewMatrix;
    slib::mat4 lightProjMatrix;

    // Combined matrix for transforming world coords to light clip space
    slib::mat4 lightSpaceMatrix;

    // Shadow bias to prevent acne (adjust based on scene scale)
    float bias = 0.05f;

    // PCF kernel size (1 = no filtering, 2 = 5x5, etc.)
    int pcfRadius = 1;

    ShadowMap(int w = 512, int h = 512)
        : width(w), height(h),
          lightViewMatrix(smath::identity()),
          lightProjMatrix(smath::identity()),
          lightSpaceMatrix(smath::identity())
    {
        depthBuffer.resize(static_cast<size_t>(w) * h, -std::numeric_limits<float>::max());
    }

    void clear() {
        std::fill(depthBuffer.begin(), depthBuffer.end(), -std::numeric_limits<float>::max());
    }

    void resize(int w, int h) {
        width = w;
        height = h;
        depthBuffer.resize(static_cast<size_t>(w) * h);
        clear();
    }

    // Store depth at pixel (x, y)
    void setDepth(int x, int y, float depth) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            depthBuffer[static_cast<size_t>(y) * width + x] = depth;
        }
    }

    // Test and set depth (like z-buffer) - returns true if depth should be written
    bool testAndSetDepth(int x, int y, float depth) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            size_t idx = static_cast<size_t>(y) * width + x;
            if (depth > depthBuffer[idx]) {
                depthBuffer[idx] = depth;
                return true;
            }
        }
        return false;
    }

    // Get depth at pixel (x, y)
    float getDepth(int x, int y) const {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return depthBuffer[static_cast<size_t>(y) * width + x];
        }
        return std::numeric_limits<float>::max();
    }

    // Build light-space matrices for shadow mapping
    // sceneCenter: approximate center of the scene being shadowed
    // sceneRadius: approximate radius encompassing shadow casters/receivers
    void buildLightMatrices(const Light& light, const slib::vec3& sceneCenter, float sceneRadius) {
        if (light.type == LightType::Directional) {
            buildDirectionalLightMatrices(light, sceneCenter, sceneRadius);
        }
        else if (light.type == LightType::Point) {
            buildPointLightMatrices(light, sceneCenter, sceneRadius);
        }
        else if (light.type == LightType::Spot) {
            buildSpotLightMatrices(light, sceneRadius);
        }

        // Pre-compute combined matrix
        lightSpaceMatrix = lightViewMatrix * lightProjMatrix;
    }

    // Sample shadow at a world position
    // Returns: 1.0 = fully lit, 0.0 = fully shadowed
    float sampleShadow(const slib::vec3& worldPos) const {
        // Transform world position to light clip space
        slib::vec4 lightSpacePos = slib::vec4(worldPos, 1.0f) * lightSpaceMatrix;

        // Perspective divide (for point/spot lights)
        if (std::abs(lightSpacePos.w) < 0.0001f) {
            return 1.0f; // Avoid division by zero
        }

        float ndcX = lightSpacePos.x / lightSpacePos.w;
        float ndcY = lightSpacePos.y / lightSpacePos.w;
        float currentDepth = lightSpacePos.z / lightSpacePos.w;

        // Map from NDC [-1,1] to texture coords [0, width/height]
        float u = (ndcX * 0.5f + 0.5f);
        float v = (ndcY * 0.5f + 0.5f);

        // Out of shadow map bounds = lit (no shadow info)
        if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f) {
            return 1.0f;
        }

        // Behind the light = lit
        if (currentDepth < -1.0f) {
            return 1.0f;
        }

        if (pcfRadius <= 1) {
            return sampleShadowSingle(u, v, currentDepth);
        } else {
            return sampleShadowPCF(u, v, currentDepth);
        }
    }

private:
    // Single sample shadow test
    float sampleShadowSingle(float u, float v, float currentDepth) const {
        int sx = static_cast<int>(u * width);
        int sy = static_cast<int>(v * height);

        // Clamp to valid range
        sx = std::clamp(sx, 0, width - 1);
        sy = std::clamp(sy, 0, height - 1);

        float storedDepth = getDepth(sx, sy);

        // If current depth (with bias) is greater than stored depth, we're in shadow
        return (currentDepth + bias > storedDepth) ? 1.0f : 0.0f;
    }

    // PCF (Percentage Closer Filtering) for soft shadow edges
    float sampleShadowPCF(float u, float v, float currentDepth) const {
        float shadow = 0.0f;
        int samples = 0;

        float texelSizeX = 1.0f / width;
        float texelSizeY = 1.0f / height;

        for (int dy = -pcfRadius; dy <= pcfRadius; dy++) {
            for (int dx = -pcfRadius; dx <= pcfRadius; dx++) {
                float sampleU = u + dx * texelSizeX;
                float sampleV = v + dy * texelSizeY;

                if (sampleU >= 0.0f && sampleU <= 1.0f &&
                    sampleV >= 0.0f && sampleV <= 1.0f) {

                    int sx = static_cast<int>(sampleU * width);
                    int sy = static_cast<int>(sampleV * height);
                    sx = std::clamp(sx, 0, width - 1);
                    sy = std::clamp(sy, 0, height - 1);

                    float storedDepth = getDepth(sx, sy);
                    shadow += (currentDepth + bias > storedDepth) ? 1.0f : 0.0f;
                    samples++;
                }
            }
        }

        return (samples > 0) ? shadow / samples : 1.0f;
    }

    void buildDirectionalLightMatrices(const Light& light, const slib::vec3& sceneCenter, float sceneRadius) {
        // Position the "light camera" far enough back along the light direction
        slib::vec3 lightDir = smath::normalize(light.direction);
        slib::vec3 lightPos = sceneCenter - lightDir * sceneRadius * 2.0f;

        // Choose an up vector that isn't parallel to light direction
        slib::vec3 up = {0.0f, 1.0f, 0.0f};
        if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
            up = {1.0f, 0.0f, 0.0f};
        }

        lightViewMatrix = smath::lookAt(lightPos, sceneCenter, up);

        // Orthographic projection sized to encompass the scene
        float size = sceneRadius * 1.5f;
        lightProjMatrix = smath::ortho(-size, size, -size, size, 0.1f, sceneRadius * 4.0f);
    }

    void buildPointLightMatrices(const Light& light, const slib::vec3& sceneCenter, float sceneRadius) {
        // For point lights, we look from the light position toward the scene center
        // Note: Full point light shadows would need 6 faces (cubemap), this is simplified

        slib::vec3 up = {0.0f, 1.0f, 0.0f};
        slib::vec3 lightDir = smath::normalize(sceneCenter - light.position);
        if (std::abs(smath::dot(lightDir, up)) > 0.99f) {
            up = {1.0f, 0.0f, 0.0f};
        }

        lightViewMatrix = smath::lookAt(light.position, sceneCenter, up);

        // Perspective projection for point light
        float fov = 90.0f * (3.14159265f / 180.0f); // 90 degree FOV
        float aspect = static_cast<float>(width) / height;
        float zNear = 1.0f;
        float zFar = light.radius * 2.0f;
        if (zFar < sceneRadius * 2.0f) {
            zFar = sceneRadius * 2.0f;
        }

        lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);
    }

    void buildSpotLightMatrices(const Light& light, float sceneRadius) {
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
