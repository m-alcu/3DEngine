#pragma once
#include <cstdint>
#include "slib.hpp"
#include "material.hpp"
#include "ZBuffer.hpp"
#include "bresenham.hpp"

template<class V>
class Polygon
{
public:
    std::vector<V> points;
    Face face;
    slib::vec3 rotatedFaceNormal;
    Material* material;
    float flatDiffuse{};

    Polygon(const Polygon& _p)
        : points(_p.points), face(_p.face), rotatedFaceNormal(_p.rotatedFaceNormal),
          material(_p.material), flatDiffuse(_p.flatDiffuse) {};
    Polygon(std::vector<V> _points, slib::vec3 _fn, Material& _material) : points(std::move(_points)), face(), rotatedFaceNormal(_fn), material(&_material) {};
    Polygon(std::vector<V> _points, slib::vec3 _fn) : points(std::move(_points)), face(), rotatedFaceNormal(_fn), material(nullptr) {};

    void drawWireframe(uint32_t color, uint32_t* pixels, int screenWidth, int screenHeight, ZBuffer* zBuffer) {
        const size_t n = points.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            auto& v0 = points[j];
            auto& v1 = points[i];
            drawBresenhamLine(v0.p_x >> 16, v0.p_y >> 16, v0.p_z,
                              v1.p_x >> 16, v1.p_y >> 16, v1.p_z,
                              pixels, color, screenWidth, screenHeight, zBuffer);
        }
    }
};
