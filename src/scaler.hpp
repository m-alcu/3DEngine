#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>

// Generic scaled blit with a sampler function for color conversion
template<typename SrcSampler>
void blitScaled(uint32_t* dst, int dstW, int dstH,
                int startX, int startY, int width, int height,
                int srcW, int srcH, SrcSampler sampler) {
    float scaleX = static_cast<float>(srcW) / width;
    float scaleY = static_cast<float>(srcH) / height;

    for (int y = 0; y < height; ++y) {
        int screenY = startY + y;
        if (screenY < 0 || screenY >= dstH) continue;
        for (int x = 0; x < width; ++x) {
            int screenX = startX + x;
            if (screenX < 0 || screenX >= dstW) continue;

            int srcX = std::clamp(static_cast<int>(x * scaleX), 0, srcW - 1);
            int srcY = std::clamp(static_cast<int>(y * scaleY), 0, srcH - 1);
            dst[screenY * dstW + screenX] = sampler(srcX, srcY);
        }
    }
}
