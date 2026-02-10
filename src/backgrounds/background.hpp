#pragma once

#include <cstdint>
#include <memory>

class Camera; // Forward declaration
class CubeMap; // Forward declaration

class Background {

    public:
        virtual void draw(uint32_t *pixels, uint16_t height, uint16_t width) = 0;

        // Extended draw with camera data (for camera-aware backgrounds like Skybox).
        // Default delegates to the basic draw().
        virtual void draw(uint32_t *pixels, uint16_t height, uint16_t width,
                          const Camera& camera, float aspectRatio) {
            draw(pixels, height, width);
        }

        bool getNeedsUpdate() {
            return needsUpdate;
        }

        void setNeedsUpdate(bool update) {
            needsUpdate = update;
        }

        virtual CubeMap* getCubeMap() { return nullptr; }

        virtual ~Background() {}

    private:
        bool needsUpdate = true;
};
