#include "skybox.hpp"
#include "../constants.hpp"
#include "../smath.hpp"
#include "../camera.hpp"
#include <cmath>

Skybox::Skybox() {
    cubemap.loadFaces(
        "resources/skybox/1/px.png",
        "resources/skybox/1/nx.png",
        "resources/skybox/1/py.png",
        "resources/skybox/1/ny.png",
        "resources/skybox/1/pz.png",
        "resources/skybox/1/nz.png"
    );
}

Skybox::Skybox(const std::string& px, const std::string& nx,
               const std::string& py, const std::string& ny,
               const std::string& pz, const std::string& nz) {
    cubemap.loadFaces(px, nx, py, ny, pz, nz);
}

void Skybox::draw(uint32_t* pixels, uint16_t height, uint16_t width) {
    std::fill_n(pixels, width * height, 0xFF000000u);
}

void Skybox::draw(uint32_t* pixels, uint16_t height, uint16_t width,
                  const Camera& camera, float aspectRatio) {

    if (!cubemap.isValid()) {
        std::fill_n(pixels, width * height, 0xFF000000u);
        return;
    }

    if (!orientationChanged(camera) && !getNeedsUpdate()) {
        return;
    }

    // Reconstruct the view rotation from camera Euler angles,
    // mirroring the math in smath::fpsview().
    const float cp = std::cos(-camera.pitch), sp = std::sin(-camera.pitch);
    const float cy = std::cos(-camera.yaw),   sy = std::sin(-camera.yaw);
    const float cr = std::cos(camera.roll),    sr = std::sin(camera.roll);

    // FPS axes after roll (x = right, y = up, z = forward):
    //   x = baseX*cr + baseY*sr
    //   y = baseY*cr - baseX*sr
    //   z = baseZ (unchanged)
    float x_x = cy * cr + sy * sp * sr;
    float x_y = cp * sr;
    float x_z = -sy * cr + cy * sp * sr;

    float y_x = sy * sp * cr - cy * sr;
    float y_y = cp * cr;
    float y_z = cy * sp * cr + sy * sr;

    float z_x = sy * cp;
    float z_y = -sp;
    float z_z = cy * cp;

    // The fpsview matrix has rows: (x.x, y.x, z.x), (x.y, y.y, z.y), ...
    // Inverse rotation = transpose, so to go from view-space to world-space:
    //   world = ndcX * col0 + ndcY * col1 + ndcZ * col2
    // where col0 = (x.x, x.y, x.z), col1 = (y.x, y.y, y.z), col2 = (z.x, z.y, z.z)

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
            cubemap.sample(worldX, worldY, worldZ, r, g, b);

            pixels[rowOffset + px] = 0xFF000000u
                | (static_cast<uint32_t>(r) << 16)
                | (static_cast<uint32_t>(g) << 8)
                |  static_cast<uint32_t>(b);
        }
    }

    lastPitch = camera.pitch;
    lastYaw   = camera.yaw;
    lastRoll  = camera.roll;
    setNeedsUpdate(false);
}

bool Skybox::orientationChanged(const Camera& camera) const {
    constexpr float eps = 1e-5f;
    return std::abs(camera.pitch - lastPitch) > eps ||
           std::abs(camera.yaw   - lastYaw)   > eps ||
           std::abs(camera.roll  - lastRoll)   > eps;
}
