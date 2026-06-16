#pragma once

#include <string>
#include <render3d/texture.hpp>

// Application-side image loading. This is the only place that turns a file path
// into a render3d::Texture; the renderer itself performs no file IO.

using namespace render3d;

namespace TextureLoader {

    // Loads an RGBA8 image from disk. Returns an empty (invalid) Texture on
    // failure. The given sampling filter is applied to the result.
    Texture load(const std::string& filename,
                 TextureFilter filter = TextureFilter::NEIGHBOUR);

} // namespace TextureLoader
