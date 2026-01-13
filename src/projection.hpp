#pragma once  

template<typename vertex>
concept has_tex = requires(vertex v) {
    v.tex;
};

template<typename vertex>
class Projection  
{  
public:  
   Projection() {}  
   
   void view(const int32_t width, const int32_t height, vertex& p, bool init) {

       if (p.broken || init) {
           float oneOverW = 1.0f / p.ndc.w;
           float sx = (p.ndc.x * oneOverW + 1.0f) * (width / 2.0f) + 0.5f; // Convert from NDC to screen coordinates
           float sy = (p.ndc.y * oneOverW + 1.0f) * (height / 2.0f) + 0.5f; // Convert from NDC to screen coordinates

           // Keep subpixel precision: 16.16 fixed-point
           constexpr float FP = 65536.0f; // 1<<16
           p.p_x = static_cast<int32_t>(sx * FP);
           p.p_y = static_cast<int32_t>(sy * FP);

           p.p_z = p.ndc.z * oneOverW; // Store the depth value in the z-buffer

           if constexpr (has_tex<vertex>) {
               if (init) {
				   p.texOverW = p.tex * oneOverW;
			   }
               else {
                   p.tex *= oneOverW;
               }
           }
	   } else {
           if constexpr (has_tex<vertex>) {
               p.tex = p.texOverW;
	       }
       }
   }
};