#pragma once

#include <cstdint>
#include <memory>

class Background {

    public:
        virtual void draw(uint32_t *pixels, uint16_t height, uint16_t width) = 0;

        bool getNeedsUpdate() {
            return needsUpdate;
        }

        void setNeedsUpdate(bool update) {
            needsUpdate = update;
        }

        virtual ~Background() {}

    private:
        bool needsUpdate = true;
};
