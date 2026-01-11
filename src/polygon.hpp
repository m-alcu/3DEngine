#pragma once
#include <cstdint>
#include "slib.hpp"
#include "material.hpp"
#include "objects/solid.hpp"

template<class V>
class Polygon
{
public:
    std::vector<V> points;
    Face face;
    slib::vec3 rotatedFaceNormal;
    slib::Material& material;
    float flatDiffuse;

    Polygon(const Polygon& _p) : points(_p.points), face(_p.face), rotatedFaceNormal(_p.rotatedFaceNormal), material(_p.material) {};
    Polygon(const std::vector<V>& _points, Face _f, slib::vec3 _fn, slib::Material& _material) : points(_points), face(_f), rotatedFaceNormal(_fn), material(_material) {};
};


