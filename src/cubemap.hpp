#pragma once

#include "texture.hpp"
#include <array>
#include <cmath>
#include <string>

enum class CubeMapFace : int {
    POSITIVE_X = 0, // Right
    NEGATIVE_X = 1, // Left
    POSITIVE_Y = 2, // Top
    NEGATIVE_Y = 3, // Bottom
    POSITIVE_Z = 4, // Front
    NEGATIVE_Z = 5  // Back
};

class CubeMap {
public:
    // Load 6 face textures by axis: px=+X, nx=-X, py=+Y, ny=-Y, pz=+Z, nz=-Z
    bool loadFaces(const std::string& px, const std::string& nx,
                   const std::string& py, const std::string& ny,
                   const std::string& pz, const std::string& nz);

    // Sample cubemap given a 3D direction vector (need not be normalized).
    // Returns color as (r, g, b) in [0, 255] range.
    void sample(float dx, float dy, float dz,
                float& r, float& g, float& b) const;

    bool isValid() const { return loaded; }

private:
    std::array<Texture, 6> faces;
    bool loaded = false;

    static Texture loadFaceTexture(const std::string& filename);
};
