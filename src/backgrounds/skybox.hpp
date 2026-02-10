#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include "background.hpp"
#include "../cubemap.hpp"

class Camera; // Forward declaration

class Skybox : public Background {
public:
    Skybox();

    // Construct with explicit face paths: px=+X, nx=-X, py=+Y, ny=-Y, pz=+Z, nz=-Z
    Skybox(const std::string& px, const std::string& nx,
           const std::string& py, const std::string& ny,
           const std::string& pz, const std::string& nz);

    void draw(uint32_t* pixels, uint16_t height, uint16_t width) override;

    void draw(uint32_t* pixels, uint16_t height, uint16_t width,
              const Camera& camera, float aspectRatio) override;

    CubeMap* getCubeMap() override { return &cubemap; }

private:
    CubeMap cubemap;

    // Cached camera orientation for change detection
    float lastPitch = -999.0f;
    float lastYaw   = -999.0f;
    float lastRoll  = -999.0f;

    bool orientationChanged(const Camera& camera) const;
};
