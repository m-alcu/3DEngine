#include "cubemap.hpp"
#include "vendor/stb/stb_image.h"
#include <iostream>

Texture CubeMap::loadFaceTexture(const std::string& filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "CubeMap: Failed to load face: " << filename << std::endl;
        return {0, 0, {}};
    }
    std::vector<unsigned char> image(data, data + (width * height * 4));
    stbi_image_free(data);

    Texture tex{width, height, std::move(image)};
    tex.setFilter(TextureFilter::BILINEAR);
    return tex;
}

bool CubeMap::loadFaces(const std::string& px, const std::string& nx,
                        const std::string& py, const std::string& ny,
                        const std::string& pz, const std::string& nz) {
    faces[static_cast<int>(CubeMapFace::POSITIVE_X)] = loadFaceTexture(px);
    faces[static_cast<int>(CubeMapFace::NEGATIVE_X)] = loadFaceTexture(nx);
    faces[static_cast<int>(CubeMapFace::POSITIVE_Y)] = loadFaceTexture(py);
    faces[static_cast<int>(CubeMapFace::NEGATIVE_Y)] = loadFaceTexture(ny);
    faces[static_cast<int>(CubeMapFace::POSITIVE_Z)] = loadFaceTexture(pz);
    faces[static_cast<int>(CubeMapFace::NEGATIVE_Z)] = loadFaceTexture(nz);

    loaded = true;
    for (const auto& face : faces) {
        if (!face.isValid()) {
            loaded = false;
            break;
        }
    }
    return loaded;
}

void CubeMap::sample(float dx, float dy, float dz,
                     float& r, float& g, float& b) const {
    if (!loaded) {
        r = 255.0f; g = 0.0f; b = 255.0f; // Magenta fallback
        return;
    }

    float absDx = std::abs(dx);
    float absDy = std::abs(dy);
    float absDz = std::abs(dz);

    int faceIndex;
    float u, v;

    if (absDx >= absDy && absDx >= absDz) {
        // X dominant
        float invAbs = 1.0f / absDx;
        if (dx > 0.0f) {
            faceIndex = static_cast<int>(CubeMapFace::POSITIVE_X);
            u = -dz * invAbs;
            v = -dy * invAbs;
        } else {
            faceIndex = static_cast<int>(CubeMapFace::NEGATIVE_X);
            u =  dz * invAbs;
            v = -dy * invAbs;
        }
    } else if (absDy >= absDx && absDy >= absDz) {
        // Y dominant
        float invAbs = 1.0f / absDy;
        if (dy > 0.0f) {
            faceIndex = static_cast<int>(CubeMapFace::POSITIVE_Y);
            u =  dx * invAbs;
            v =  dz * invAbs;
        } else {
            faceIndex = static_cast<int>(CubeMapFace::NEGATIVE_Y);
            u =  dx * invAbs;
            v = -dz * invAbs;
        }
    } else {
        // Z dominant
        float invAbs = 1.0f / absDz;
        if (dz > 0.0f) {
            faceIndex = static_cast<int>(CubeMapFace::POSITIVE_Z);
            u =  dx * invAbs;
            v = -dy * invAbs;
        } else {
            faceIndex = static_cast<int>(CubeMapFace::NEGATIVE_Z);
            u = -dx * invAbs;
            v = -dy * invAbs;
        }
    }

    // Map from [-1, 1] to [0, 1]
    u = 0.5f * u + 0.5f;
    v = 0.5f * v + 0.5f;

    faces[faceIndex].sample(u, v, r, g, b);
}
