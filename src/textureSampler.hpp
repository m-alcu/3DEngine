#pragma once
#include "slib.hpp"
#include "color.hpp"

template<typename Vertex>
class TextureSampler
{
public:
    TextureSampler(const Vertex& vRaster, const slib::texture& texture, slib::TextureFilter filter)
        : vRaster_(vRaster), texture_(texture), filter_(filter) {
    }

    // Sample the texture at (u, v, w) coordinates
    Color sample(float diff, float r_spec, float g_spec, float b_spec) const
    {
        float w = 1 / vRaster_.tex.w;
        if (filter_ == slib::TextureFilter::BILINEAR) {
            float r, g, b;
            smath::sampleBilinear(texture_, vRaster_.tex.x * w, vRaster_.tex.y * w, r, g, b);
            return Color(r * diff + r_spec, g * diff + g_spec, b * diff + b_spec);
        }
        else {
            int r, g, b;
            smath::sampleNearest(texture_, vRaster_.tex.x * w, vRaster_.tex.y * w, r, g, b);
            return Color(r * diff + r_spec, g * diff + g_spec, b * diff + b_spec);
        }
    }

private:
    const slib::texture& texture_;
    slib::TextureFilter filter_;
	const Vertex& vRaster_;
};