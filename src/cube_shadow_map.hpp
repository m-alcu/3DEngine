#pragma once

#include "z_buffer.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "light.hpp"
#include <array>
#include <memory>
#include <cstdio>

// Cubemap face enumeration matching standard cube face ordering
enum class CubeShadowFace : int {
    POSITIVE_X = 0, // Right
    NEGATIVE_X = 1, // Left
    POSITIVE_Y = 2, // Top
    NEGATIVE_Y = 3, // Bottom
    POSITIVE_Z = 4, // Front
    NEGATIVE_Z = 5  // Back
};

// Cube shadow map for omnidirectional shadows (point lights)
// Stores 6 depth faces covering all directions around a light
class CubeShadowMap {
public:
    int faceSize; // Width/height of each face (square)
    std::array<std::unique_ptr<ZBuffer>, 6> faces;
    std::array<slib::mat4, 6> lightViewMatrices;
    slib::mat4 lightProjMatrix; // Shared projection for all faces
    std::array<slib::mat4, 6> lightSpaceMatrices; // Combined view*proj for each face
    float zNear = 0.1f;
    float zFar = 100.0f;
    float maxSlopeBias = 10.0f;

    CubeShadowMap(int size = 512)
        : faceSize(size), 
          lightProjMatrix(smath::identity()),
          lightViewMatrices{smath::identity(), smath::identity(), smath::identity(), 
                            smath::identity(), smath::identity(), smath::identity()},
          lightSpaceMatrices{smath::identity(), smath::identity(), smath::identity(), 
                             smath::identity(), smath::identity(), smath::identity()} {
        for (int i = 0; i < 6; ++i) {
            faces[i] = std::make_unique<ZBuffer>(size, size);
            lightViewMatrices[i] = smath::identity();
            lightSpaceMatrices[i] = smath::identity();
        }
        clear();
    }

    void clear() {
        for (auto& face : faces) {
            if (face) {
                face->Clear();
            }
        }
    }

    // Convert NDC z to normalized linear depth [0, 1]
    // NDC z ranges from -1 (near) to +1 (far)
    // Returns linearDepth / zFar, so 0 = at light, 1 = at zFar
    float ndcToLinearDepth(float z_ndc) const {
        float linearDepth = 2.0f * zNear * zFar /
            (zFar + zNear - z_ndc * (zFar - zNear));
        return linearDepth / zFar;
    }

    void resize(int size) {
        faceSize = size;
        for (auto& face : faces) {
            if (face) {
                face->Resize(size, size);
            }
        }
        clear();
    }

    // Get depth from specific face at position
    float getDepth(CubeShadowFace face, int pos) const {
        return faces[static_cast<int>(face)]->Get(pos);
    }

    // Test and set depth for specific face
    bool testAndSetDepth(CubeShadowFace face, int pos, float depth) {
        return faces[static_cast<int>(face)]->TestAndSet(pos, depth);
    }

    // Determine which cube face to use based on direction from light to point
    CubeShadowFace selectFace(const slib::vec3& dir) const {
        float absX = std::abs(dir.x);
        float absY = std::abs(dir.y);
        float absZ = std::abs(dir.z);

        if (absX >= absY && absX >= absZ) {
            return dir.x > 0 ? CubeShadowFace::POSITIVE_X : CubeShadowFace::NEGATIVE_X;
        } else if (absY >= absX && absY >= absZ) {
            return dir.y > 0 ? CubeShadowFace::POSITIVE_Y : CubeShadowFace::NEGATIVE_Y;
        } else {
            return dir.z > 0 ? CubeShadowFace::POSITIVE_Z : CubeShadowFace::NEGATIVE_Z;
        }
    }

    // Build 6 view matrices for cubemap faces centered at lightPos
    void buildCubemapMatrices(const slib::vec3& lightPos, float _zNear = 0.1f, float _zFar = 100.0f) {
        zNear = _zNear;
        zFar = _zFar;
        // 90-degree FOV perspective for each face
        float aspect = 1.0f; // Square faces
        float fov = 90.0f * 3.14159265f / 180.0f; // 90 degrees in radians
        lightProjMatrix = smath::perspective(zFar, zNear, aspect, fov);


        // Define 6 views (right, left, up, down, front, back)
        // Each face looks outward from the light position

        // +X (Right)
        lightViewMatrices[0] = smath::lookAt(lightPos, lightPos + slib::vec3{1, 0, 0}, {0, -1, 0});
        // -X (Left)
        lightViewMatrices[1] = smath::lookAt(lightPos, lightPos + slib::vec3{-1, 0, 0}, {0, -1, 0});
        // +Y (Top)
        lightViewMatrices[2] = smath::lookAt(lightPos, lightPos + slib::vec3{0, 1, 0}, {0, 0, 1});
        // -Y (Bottom)
        lightViewMatrices[3] = smath::lookAt(lightPos, lightPos + slib::vec3{0, -1, 0}, {0, 0, -1});
        // +Z (Front)
        lightViewMatrices[4] = smath::lookAt(lightPos, lightPos + slib::vec3{0, 0, 1}, {0, -1, 0});
        // -Z (Back)
        lightViewMatrices[5] = smath::lookAt(lightPos, lightPos + slib::vec3{0, 0, -1}, {0, -1, 0});

        // Precompute combined matrices
        for (int i = 0; i < 6; ++i) {
            lightSpaceMatrices[i] = lightViewMatrices[i] * lightProjMatrix;
        }
    }

    // Sample shadow from the appropriate cube face
    // Returns: 1.0 = fully lit, 0.0 = fully shadowed
    // Uses linear distance comparison for uniform precision
    // cosTheta: dot(surfaceNormal, lightDirection) for slope-scaled bias
    float sampleShadow(const slib::vec3& worldPos, const slib::vec3& lightPos,
                      float cosTheta, int pcfRadius) const {
        // Calculate direction from light to fragment
        slib::vec3 lightToFrag = worldPos - lightPos;

        // Select appropriate face
        CubeShadowFace face = selectFace(lightToFrag);
        int faceIdx = static_cast<int>(face);

        // Transform world position to light clip space for this face
        slib::vec4 lightSpacePos = slib::vec4(worldPos, 1.0f) * lightSpaceMatrices[faceIdx];

        // Perspective divide
        if (std::abs(lightSpacePos.w) < 0.0001f) {
            return 1.0f;
        }

        float oneOverW = 1.0f / lightSpacePos.w;
        float ndcX = lightSpacePos.x * oneOverW;
        float ndcY = lightSpacePos.y * oneOverW;

        // Map from NDC [-1,1] to texture coords [0, faceSize]
        int sx = static_cast<int>((ndcX * 0.5f + 0.5f) * faceSize + 0.5f);
        int sy = static_cast<int>((-ndcY * 0.5f + 0.5f) * faceSize + 0.5f);

        // Convert current depth to linear space: w_clip = -z_view
        // ndcToLinearDepth produces the same value (linearDepth/zFar = w_clip/zFar)
        float currentDepth = lightSpacePos.w / zFar;

        // Beyond light range or behind light = lit
        if (currentDepth > 1.0f || lightSpacePos.w < 0.0f) {
            return 1.0f;
        }

        // Slope-scaled bias in normalized linear depth space
        float texelDepth = 2.0f * currentDepth / faceSize;
        cosTheta = std::clamp(cosTheta, 0.0f, 1.0f);
        float slopeFactor = (cosTheta > 0.01f)
            ? std::min(1.0f / cosTheta, maxSlopeBias)
            : maxSlopeBias;
        float bias = texelDepth * slopeFactor;

        if (pcfRadius < 1) {
            return sampleShadowSingle(face, sx, sy, currentDepth, bias);
        } else {
            return sampleShadowPCF(face, sx, sy, currentDepth, bias, pcfRadius);
        }
    }

private:
    float sampleShadowSingle(CubeShadowFace face, int sx, int sy,
                            float currentDepth, float bias) const {
        // Out of bounds = lit
        if (sx < 0 || sx >= faceSize || sy < 0 || sy >= faceSize) {
            return 1.0f;
        }

        // Stored as NDC p_z, convert to linear at read time
        float storedDepth = ndcToLinearDepth(getDepth(face, sy * faceSize + sx));
        return (currentDepth - bias < storedDepth) ? 1.0f : 0.0f;
    }

    float sampleShadowPCF(CubeShadowFace face, int sx, int sy,
                         float currentDepth, float bias, int pcfRadius) const {
        int shadow = 0;
        int samples = 0;

        for (int dy = -pcfRadius; dy <= pcfRadius; dy++) {
            int dsy = sy + dy;
            if (dsy < 0 || dsy >= faceSize) continue;

            for (int dx = -pcfRadius; dx <= pcfRadius; dx++) {
                int dsx = sx + dx;
                if (dsx < 0 || dsx >= faceSize) continue;

                // Stored as NDC p_z, convert to linear at read time
                float storedDepth = ndcToLinearDepth(getDepth(face, dsy * faceSize + dsx));
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
};
