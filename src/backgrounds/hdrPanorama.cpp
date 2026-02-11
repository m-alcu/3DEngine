#include "hdrPanorama.hpp"
#include "../constants.hpp"
#include "../smath.hpp"
#include "../camera.hpp"
#include "../vendor/stb/stb_image.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

HdrPanorama::HdrPanorama() {
    load("resources/hdrs/HDR_multi_nebulae_1.hdr");
}

HdrPanorama::HdrPanorama(const std::string& path) {
    load(path);
}

HdrPanorama::~HdrPanorama() {
    if (hdrData) {
        stbi_image_free(hdrData);
    }
}

bool HdrPanorama::load(const std::string& path) {
    int channels;
    hdrData = stbi_loadf(path.c_str(), &imgWidth, &imgHeight, &channels, 3);
    if (!hdrData) {
        std::fprintf(stderr, "HdrPanorama: failed to load '%s': %s\n",
                     path.c_str(), stbi_failure_reason());
        return false;
    }
    return true;
}

void HdrPanorama::sampleEquirectangular(float dx, float dy, float dz,
                                        float& r, float& g, float& b) const {
    // Direction to equirectangular UV
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    float nx = dx / len;
    float ny = dy / len;
    float nz = dz / len;

    float u = 0.5f + std::atan2(nz, nx) / (2.0f * PI);
    float v = 0.5f - std::asin(std::clamp(ny, -1.0f, 1.0f)) / PI;

    // Bilinear interpolation
    float fx = u * (imgWidth - 1);
    float fy = v * (imgHeight - 1);

    int x0 = static_cast<int>(fx);
    int y0 = static_cast<int>(fy);
    int x1 = std::min(x0 + 1, imgWidth - 1);
    int y1 = std::min(y0 + 1, imgHeight - 1);

    float tx = fx - x0;
    float ty = fy - y0;

    // Wrap x horizontally for seamless panorama
    if (x0 < 0) x0 += imgWidth;
    if (x1 >= imgWidth) x1 = 0;

    const float* p00 = &hdrData[(y0 * imgWidth + x0) * 3];
    const float* p10 = &hdrData[(y0 * imgWidth + x1) * 3];
    const float* p01 = &hdrData[(y1 * imgWidth + x0) * 3];
    const float* p11 = &hdrData[(y1 * imgWidth + x1) * 3];

    float w00 = (1.0f - tx) * (1.0f - ty);
    float w10 = tx * (1.0f - ty);
    float w01 = (1.0f - tx) * ty;
    float w11 = tx * ty;

    r = p00[0] * w00 + p10[0] * w10 + p01[0] * w01 + p11[0] * w11;
    g = p00[1] * w00 + p10[1] * w10 + p01[1] * w01 + p11[1] * w11;
    b = p00[2] * w00 + p10[2] * w10 + p01[2] * w01 + p11[2] * w11;
}

void HdrPanorama::draw(uint32_t* pixels, uint16_t height, uint16_t width) {
    std::fill_n(pixels, width * height, 0xFF000000u);
}

void HdrPanorama::draw(uint32_t* pixels, uint16_t height, uint16_t width,
                       const Camera& camera, float aspectRatio) {

    if (!hdrData) {
        std::fill_n(pixels, width * height, 0xFF000000u);
        return;
    }

    if (!orientationChanged(camera) && !getNeedsUpdate()) {
        return;
    }

    // Reconstruct view rotation from camera Euler angles (same as Skybox)
    const float cp = std::cos(-camera.pitch), sp = std::sin(-camera.pitch);
    const float cy = std::cos(-camera.yaw),   sy = std::sin(-camera.yaw);
    const float cr = std::cos(camera.roll),    sr = std::sin(camera.roll);

    float x_x = cy * cr + sy * sp * sr;
    float x_y = cp * sr;
    float x_z = -sy * cr + cy * sp * sr;

    float y_x = sy * sp * cr - cy * sr;
    float y_y = cp * cr;
    float y_z = cy * sp * cr + sy * sr;

    float z_x = sy * cp;
    float z_y = -sp;
    float z_z = cy * cp;

    float tanFov = std::tan(camera.viewAngle * RAD);
    float invW = 1.0f / static_cast<float>(width);
    float invH = 1.0f / static_cast<float>(height);

    for (int py = 0; py < height; ++py) {
        float ndcY = (1.0f - 2.0f * (py + 0.5f) * invH) * tanFov;
        int rowOffset = py * width;

        for (int px = 0; px < width; ++px) {
            float ndcX = (2.0f * (px + 0.5f) * invW - 1.0f) * aspectRatio * tanFov;
            constexpr float ndcZ = -1.0f;

            float worldX = ndcX * x_x + ndcY * y_x + ndcZ * z_x;
            float worldY = ndcX * x_y + ndcY * y_y + ndcZ * z_y;
            float worldZ = ndcX * x_z + ndcY * y_z + ndcZ * z_z;

            float r, g, b;
            sampleEquirectangular(worldX, worldY, worldZ, r, g, b);

            // Reinhard tone mapping
            r = 1.0f - std::exp(-r * exposure);
            g = 1.0f - std::exp(-g * exposure);
            b = 1.0f - std::exp(-b * exposure);

            uint32_t ir = static_cast<uint32_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
            uint32_t ig = static_cast<uint32_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
            uint32_t ib = static_cast<uint32_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));

            pixels[rowOffset + px] = 0xFF000000u | (ir << 16) | (ig << 8) | ib;
        }
    }

    lastPitch = camera.pitch;
    lastYaw   = camera.yaw;
    lastRoll  = camera.roll;
    setNeedsUpdate(false);
}

bool HdrPanorama::orientationChanged(const Camera& camera) const {
    constexpr float eps = 1e-5f;
    return std::abs(camera.pitch - lastPitch) > eps ||
           std::abs(camera.yaw   - lastYaw)   > eps ||
           std::abs(camera.roll  - lastRoll)   > eps;
}
