#pragma once  
#include "scene.hpp"

template<typename vertex>
concept has_tex = requires(vertex v) {
    v.tex;
};

template<typename vertex>
class Projection  
{  
public:  
   Projection() {}  
   
   void view(const Scene& scene, vertex& p) {
       float oneOverW = 1.0f / p.ndc.w;
       float sx = (p.ndc.x * oneOverW + 1.0f) * (scene.screen.width / 2.0f) + 0.5f; // Convert from NDC to screen coordinates
       float sy = (p.ndc.y * oneOverW + 1.0f) * (scene.screen.height / 2.0f) + 0.5f; // Convert from NDC to screen coordinates

       // Keep subpixel precision: 16.16 fixed-point
       constexpr float FP = 65536.0f; // 1<<16
       p.p_x = static_cast<int32_t>(sx * FP);
       p.p_y = static_cast<int32_t>(sy * FP);

       p.p_z = p.ndc.z * oneOverW; // Store the depth value in the z-buffer

       if constexpr (has_tex<vertex>) {
           p.texOverW = p.tex * oneOverW;
       }
   }

   void viewConditionalBroken(const Scene& scene, vertex& p) {

       if (p.broken) {
           float oneOverW = 1.0f / p.ndc.w;
           float sx = (p.ndc.x * oneOverW + 1.0f) * (scene.screen.width / 2.0f) + 0.5f; // Convert from NDC to screen coordinates
           float sy = (p.ndc.y * oneOverW + 1.0f) * (scene.screen.height / 2.0f) + 0.5f; // Convert from NDC to screen coordinates

           // Keep subpixel precision: 16.16 fixed-point
           constexpr float FP = 65536.0f; // 1<<16
           p.p_x = static_cast<int32_t>(sx * FP);
           p.p_y = static_cast<int32_t>(sy * FP);

           p.p_z = p.ndc.z * oneOverW; // Store the depth value in the z-buffer

           if constexpr (has_tex<vertex>) {
               p.tex *= oneOverW;
           }
	   } else {
           if constexpr (has_tex<vertex>) {
               p.tex = p.texOverW;
	       }
       }
   }
};