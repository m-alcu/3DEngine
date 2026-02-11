#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include "background.hpp"

class Camera;

class HdrPanorama : public Background {
public:
    HdrPanorama();
    explicit HdrPanorama(const std::string& path);
    ~HdrPanorama();

    HdrPanorama(const HdrPanorama&) = delete;
    HdrPanorama& operator=(const HdrPanorama&) = delete;

    void draw(uint32_t* pixels, uint16_t height, uint16_t width) override;
    void draw(uint32_t* pixels, uint16_t height, uint16_t width,
              const Camera& camera, float aspectRatio) override;

    float exposure = 10.0f;

private:
    float* hdrData = nullptr;
    int imgWidth = 0;
    int imgHeight = 0;

    float lastPitch = -999.0f;
    float lastYaw   = -999.0f;
    float lastRoll  = -999.0f;

    bool load(const std::string& path);
    void sampleEquirectangular(float dx, float dy, float dz,
                               float& r, float& g, float& b) const;
    bool orientationChanged(const Camera& camera) const;
};
