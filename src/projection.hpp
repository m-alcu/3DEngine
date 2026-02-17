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

       if (p.dirty || init) {
           constexpr float FP = 65536.0f;
           float oneOverW = 1.0f / p.ndc.w;
           float halfW_FP = width  * (0.5f * FP);
           float halfH_FP = height * (0.5f * FP);
           float cxFP = (width  * 0.5f + 0.5f) * FP;
           float cyFP = (height * 0.5f + 0.5f) * FP;

           p.p_x = static_cast<int32_t>(p.ndc.x * oneOverW * halfW_FP + cxFP);
           p.p_y = static_cast<int32_t>(-p.ndc.y * oneOverW * halfH_FP + cyFP);
           p.p_z = p.ndc.z * oneOverW;
       }
       return true;
   }

    static bool texturedView(const int32_t width, const int32_t height, vertex& p, bool init) {
       if (p.ndc.w <= 0.0001f) {
           return false;
       }

       if (p.dirty || init) {
            constexpr float FP = 65536.0f;
            float oneOverW = 1.0f / p.ndc.w;
            float halfW_FP = width  * (0.5f * FP);
            float halfH_FP = height * (0.5f * FP);
            float cxFP = (width  * 0.5f + 0.5f) * FP;
            float cyFP = (height * 0.5f + 0.5f) * FP;

            p.p_x = static_cast<int32_t>(p.ndc.x * oneOverW * halfW_FP + cxFP);
            p.p_y = static_cast<int32_t>(-p.ndc.y * oneOverW * halfH_FP + cyFP);

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