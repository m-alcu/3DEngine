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
    Material* material;
    float flatDiffuse;

    Polygon(const Polygon& _p) : points(_p.points), face(_p.face), rotatedFaceNormal(_p.rotatedFaceNormal), material(_p.material) {};
    Polygon(std::vector<V> _points, Face _f, slib::vec3 _fn, Material& _material) : points(std::move(_points)), face(_f), rotatedFaceNormal(_fn), material(&_material) {};
    Polygon(std::vector<V> _points, slib::vec3 _fn) : points(std::move(_points)), face(), rotatedFaceNormal(_fn), material(nullptr) {};
};


