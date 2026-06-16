#pragma once

#include <memory>
#include <string>
#include <render3d/backgrounds/background.hpp>

// Application-side construction of backgrounds. This is where the files a
// background needs (skybox faces, HDR panorama, image/twister textures) are
// loaded from disk; the background classes themselves perform no file IO.

using namespace render3d;

namespace BackgroundFactory {

    // Construct a background of the given type using built-in default resource
    // paths. Returns nullptr for an unknown type.
    std::unique_ptr<Background> create(BackgroundType type);

    // Skybox from 6 explicit face image paths (px=+X, nx=-X, py=+Y, ny=-Y, pz=+Z, nz=-Z).
    std::unique_ptr<Background> createSkybox(const std::string& px, const std::string& nx,
                                             const std::string& py, const std::string& ny,
                                             const std::string& pz, const std::string& nz);

    // Equirectangular HDR panorama from an .hdr file path.
    std::unique_ptr<Background> createHdrPanorama(const std::string& path);

} // namespace BackgroundFactory
