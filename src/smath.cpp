//
// Created by Steve Wheeler on 23/08/2023.
//

#include "smath.hpp"
#include "constants.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace smath
{
    float distance(const slib::vec3& vec)
    {
        return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    }

    slib::vec3 centroid(const std::vector<slib::vec3>& points)
    {
        slib::vec3 result({0, 0, 0});
        for (const auto& v : points)
            result += v;
        return result / points.size();
    }

    slib::vec3 normalize(slib::vec3 vec)
    {
        return vec / distance(vec);
    }

    float dot(const slib::vec3& v1, const slib::vec3& v2)
    {
        // Care: assumes both vectors have been normalised previously.
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    slib::vec3 cross(const slib::vec3& v1, const slib::vec3& v2) {
        return slib::vec3({
            v1.y * v2.z - v1.z * v2.y,
            v1.z * v2.x - v1.x * v2.z, // FIXED HERE
            v1.x * v2.y - v1.y * v2.x
        });
    }

/*
    Viewer
    (Camera)        zNear                      zFar
       |              |                          |
       v              v                          v
       +--------------+--------------------------+
                      \                        /
                       \   Visible Volume     /
                        \     (Frustum)      /
                         \                  /
                          \                /
                           \              /
                            +------------+
                          Projection (Perspective)
 */   

    slib::mat4 perspective(const float zFar, const float zNear, const float aspect, const float fov)
    {
        const float yScale = 1 / tanf(fov);
        const float xScale = yScale / aspect;
        const float nearmfar = zNear - zFar;

        slib::mat4 mat(
            {{xScale, 0, 0, 0},
             {0, yScale, 0, 0},
             {0, 0, (zFar + zNear) / nearmfar, -1},
             {0, 0, 2 * zFar * zNear / nearmfar, 0}});
        return mat;
    }

    slib::mat4 view(const slib::vec3& eye, const slib::vec3& target, const slib::vec3& up)
    {
        slib::vec3 zaxis = normalize(eye - target);
        slib::vec3 xaxis = normalize(cross(up, zaxis));
        slib::vec3 yaxis = cross(zaxis, xaxis);

        slib::mat4 viewMatrix(
            {{xaxis.x, yaxis.x, zaxis.x, 0},
             {xaxis.y, yaxis.y, zaxis.y, 0},
             {xaxis.z, yaxis.z, zaxis.z, 0},
             {-dot(xaxis, eye), -dot(yaxis, eye), -dot(zaxis, eye), 1}});

        return viewMatrix;
    }

    slib::mat4 fpsview(const slib::vec3& eye, float pitch, float yaw, float roll)
    {

        const float cp = std::cos(pitch), sp = std::sin(pitch);
        const float cy = std::cos(yaw), sy = std::sin(yaw);
        const float cr = std::cos(roll), sr = std::sin(roll);

        // Base FPS axes from yaw/pitch (your original)
        slib::vec3 xaxis = { cy,        0.0f, -sy }; // right
        slib::vec3 yaxis = { sy * sp,     cp,    cy * sp }; // up
        slib::vec3 zaxis = { sy * cp,    -sp,    cy * cp }; // forward (view dir)

        // Roll around the forward axis (zaxis): rotate (x,y) in their plane
        slib::vec3 x = xaxis * cr + yaxis * sr;
        slib::vec3 y = yaxis * cr - xaxis * sr;
        const slib::vec3& z = zaxis;

        // (Optional) re-orthonormalize if you worry about drift:
        // x = normalize(x); y = normalize(y - z*dot(y,z));  x = normalize(cross(y,z)); // etc.

        slib::mat4 view(
            { { x.x,  y.x,  z.x, 0.0f },
             { x.y,  y.y,  z.y, 0.0f },
             { x.z,  y.z,  z.z, 0.0f },
             { -dot(x, eye), -dot(y, eye), -dot(z, eye), 1.0f } });

        return view;
    }


    slib::mat4 rotation(const slib::vec3& eulerAngles)
    {
        const float xrad = eulerAngles.x * RAD;
        const float yrad = eulerAngles.y * RAD;
        const float zrad = eulerAngles.z * RAD;
        const float axc = std::cos(xrad);
        const float axs = std::sin(xrad);
        const float ayc = std::cos(yrad);
        const float ays = -std::sin(yrad);
        const float azc = std::cos(zrad);
        const float azs = -std::sin(zrad);

        slib::mat4 rotateX({{1, 0, 0, 0}, {0, axc, axs, 0}, {0, -axs, axc, 0}, {0, 0, 0, 1}});

        slib::mat4 rotateY({{ayc, 0, -ays, 0}, {0, 1, 0, 0}, {ays, 0, ayc, 0}, {0, 0, 0, 1}});

        slib::mat4 rotateZ({{azc, azs, 0, 0}, {-azs, azc, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}});

        return rotateZ * rotateX * rotateY;
    }

    slib::mat4 scale(const slib::vec3& scale)
    {
        return slib::mat4({{scale.x, 0, 0, 0}, {0, scale.y, 0, 0}, {0, 0, scale.z, 0}, {0, 0, 0, 1}});
    }

    slib::mat4 translation(const slib::vec3& translation)
    {
        return slib::mat4(
            {{1, 0, 0, translation.x}, {0, 1, 0, translation.y}, {0, 0, 1, translation.z}, {0, 0, 0, 1}});
    }

    slib::mat4 identity()
    {
        return slib::mat4({{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}});
    }

    void sampleNearest(const slib::texture& tex, float u, float v, int& r, int& g, int& b)
    {
        int tx = static_cast<int>(u * tex.w);
        if (tx == tex.w) tx = tex.w - 1;

        int ty = static_cast<int>(v * tex.h);
        if (ty == tex.h) ty = tex.h - 1;

		//we assume always bpp = 4 (RGBA)

        const uint32_t* row32 = reinterpret_cast<const uint32_t*>(&tex.data[ty * tex.rowStride]);
        uint32_t px = row32[tx];

        r = (px) & 0xFF;
        g = (px >> 8) & 0xFF;
        b = (px >> 16) & 0xFF;
    }

    /**
     * Bilinear sampler for RGBA8 textures.
     *
     * Optimizations applied:
     *  - Computes integer texel coordinates once (floor), clamps only twice.
     *  - Uses precomputed row stride to avoid repeated multiplications.
     *  - Assumes bpp == 4 (RGBA8), so pixels are read as 32-bit words.
     *  - Loads four neighboring pixels with 32-bit reads, then unpacks channels.
     *  - Performs bilinear interpolation as two horizontal lerps + one vertical lerp
     *    instead of four weighted blends, reducing multiplications.
     *
     * Input (u,v) are normalized [0,1] texture coordinates.
     * Output r,g,b are floating-point channel values in [0,255].
     */
    void sampleBilinear(const slib::texture& tex, float u, float v, float& r, float& g, float& b)
    {
        // Map to texel space and center on texel centers (-0.5)
        float xf = u * tex.w - 0.5f;
        float yf = v * tex.h - 0.5f;

        // Floor once; clamp to keep (x+1,y+1) in-bounds
        int x = static_cast<int>(std::floor(xf));
        int y = static_cast<int>(std::floor(yf));
        if (x < 0) x = 0; else if (x > tex.w - 2) x = tex.w - 2;
        if (y < 0) y = 0; else if (y > tex.h - 2) y = tex.h - 2;

        float fx = xf - x;          // frac in [0,1)
        float fy = yf - y;

        // Precompute row bases (in bytes)
        const uint8_t* rowT = tex.data.data() + y * tex.rowStride;
        const uint8_t* rowB = rowT + tex.rowStride;

        // Assume RGBA8 (bpp = 4)
        const uint32_t* rowT32 = reinterpret_cast<const uint32_t*>(rowT);
        const uint32_t* rowB32 = reinterpret_cast<const uint32_t*>(rowB);

        // Load 4 neighboring pixels (safe for unaligned on x86; use memcpy if you prefer strict aliasing safety)
        uint32_t p00 = rowT32[x];     // (x,   y)
        uint32_t p10 = rowT32[x + 1]; // (x+1, y)
        uint32_t p01 = rowB32[x];     // (x,   y+1)
        uint32_t p11 = rowB32[x + 1]; // (x+1, y+1)

        // Horizontal lerp on top and bottom rows per channel (8 muls total), then vertical lerp (3 more)
        // Extract channels as 0..255 ints
        auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };

        float rT = lerp(static_cast<float>(p00 & 0xFF),
            static_cast<float>(p10 & 0xFF), fx);
        float gT = lerp(static_cast<float>((p00 >> 8) & 0xFF),
            static_cast<float>((p10 >> 8) & 0xFF), fx);
        float bT = lerp(static_cast<float>((p00 >> 16) & 0xFF),
            static_cast<float>((p10 >> 16) & 0xFF), fx);

        float rB = lerp(static_cast<float>(p01 & 0xFF),
            static_cast<float>(p11 & 0xFF), fx);
        float gB = lerp(static_cast<float>((p01 >> 8) & 0xFF),
            static_cast<float>((p11 >> 8) & 0xFF), fx);
        float bB = lerp(static_cast<float>((p01 >> 16) & 0xFF),
            static_cast<float>((p11 >> 16) & 0xFF), fx);

        r = lerp(rT, rB, fy);
        g = lerp(gT, gB, fy);
        b = lerp(bT, bB, fy);
    }

} // namespace smath