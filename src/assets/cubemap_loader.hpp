#pragma once

#include <array>
#include <string>
#include <render3d/cubemap.hpp>
#include <render3d/texture.hpp>
#include "texture_loader.hpp"

// Application-side cubemap loading. Loads 6 face images from disk and injects
// them into a render3d::CubeMap.

using namespace render3d;

namespace CubeMapLoader {

    // Loads faces by axis: px=+X, nx=-X, py=+Y, ny=-Y, pz=+Z, nz=-Z.
    inline CubeMap load(const std::string& px, const std::string& nx,
                        const std::string& py, const std::string& ny,
                        const std::string& pz, const std::string& nz) {
        std::array<Texture, 6> faces;
        faces[static_cast<int>(CubeMapFace::POSITIVE_X)] = TextureLoader::load(px, TextureFilter::BILINEAR);
        faces[static_cast<int>(CubeMapFace::NEGATIVE_X)] = TextureLoader::load(nx, TextureFilter::BILINEAR);
        faces[static_cast<int>(CubeMapFace::POSITIVE_Y)] = TextureLoader::load(py, TextureFilter::BILINEAR);
        faces[static_cast<int>(CubeMapFace::NEGATIVE_Y)] = TextureLoader::load(ny, TextureFilter::BILINEAR);
        faces[static_cast<int>(CubeMapFace::POSITIVE_Z)] = TextureLoader::load(pz, TextureFilter::BILINEAR);
        faces[static_cast<int>(CubeMapFace::NEGATIVE_Z)] = TextureLoader::load(nz, TextureFilter::BILINEAR);

        CubeMap cubemap;
        cubemap.setFaces(std::move(faces));
        return cubemap;
    }

} // namespace CubeMapLoader
