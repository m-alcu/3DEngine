#pragma once

#include "slib.hpp"

template<typename vertex>
class Projection
{
public:
   Projection() {}

   // Project vertex to screen coordinates in 16.16 fixed-point format
   // Returns false if point is behind camera (w <= 0)
   static bool view(const int32_t width, const int32_t height, vertex& p, bool init) {
       if (p.ndc.w <= 0.0001f) {
           return false;
       }

       if (p.broken || init) {
           float oneOverW = 1.0f / p.ndc.w;
           float sx = (p.ndc.x * oneOverW * 0.5f + 0.5f) * width + 0.5f;
           float sy = (-p.ndc.y * oneOverW * 0.5f + 0.5f) * height + 0.5f;

           // Keep subpixel precision: 16.16 fixed-point
           constexpr float FP = 65536.0f; // 1<<16
           p.p_x = static_cast<int32_t>(sx * FP);
           p.p_y = static_cast<int32_t>(sy * FP);

           p.p_z = p.ndc.z * oneOverW;
       }
       return true;
   }

    static bool texturedView(const int32_t width, const int32_t height, vertex& p, bool init) {
       if (p.ndc.w <= 0.0001f) {
           return false;
       }

       if (p.broken || init) {
            float oneOverW = 1.0f / p.ndc.w;
            float sx = (p.ndc.x * oneOverW * 0.5f + 0.5f) * width + 0.5f;
            float sy = (-p.ndc.y * oneOverW * 0.5f + 0.5f) * height + 0.5f;

            // Keep subpixel precision: 16.16 fixed-point
            constexpr float FP = 65536.0f; // 1<<16
            p.p_x = static_cast<int32_t>(sx * FP);
            p.p_y = static_cast<int32_t>(sy * FP);

            p.p_z = p.ndc.z * oneOverW;

            if (init) {
                p.texOverW = p.tex * oneOverW;
            } else {
                p.tex *= oneOverW;
            }
       } else {
            p.tex = p.texOverW;
       }
       return true;
   }
};