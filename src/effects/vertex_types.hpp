#pragma once
#include "../slib.hpp"

namespace vertex {

class Flat {
public:
    Flat() {}

    Flat(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::vec3 _world, bool _dirty,
         slib::vec3 _worldOverW = {}, float _oneOverW = 1.0f)
        : p_x(px), p_y(py), p_z(pz), clip(vp), world(_world), dirty(_dirty),
          worldOverW(_worldOverW), oneOverW(_oneOverW) {}

    Flat operator+(const Flat &v) const {
        return Flat(p_x + v.p_x, p_y, p_z + v.p_z, clip + v.clip, world + v.world, true,
                    worldOverW + v.worldOverW, oneOverW + v.oneOverW);
    }

    Flat operator-(const Flat &v) const {
        return Flat(p_x - v.p_x, p_y, p_z - v.p_z, clip - v.clip, world - v.world, true,
                    worldOverW - v.worldOverW, oneOverW - v.oneOverW);
    }

    Flat operator*(const float &rhs) const {
        return Flat(p_x * rhs, p_y, p_z * rhs, clip * rhs, world * rhs, true,
                    worldOverW * rhs, oneOverW * rhs);
    }

    Flat &operator+=(const Flat &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        clip += v.clip;
        world += v.world;
        worldOverW += v.worldOverW;
        oneOverW += v.oneOverW;
        return *this;
    }

    Flat &vraster(const Flat &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        worldOverW += v.worldOverW;
        oneOverW += v.oneOverW;
        return *this;
    }

    Flat &hraster(const Flat &v) {
        p_z += v.p_z;
        worldOverW += v.worldOverW;
        oneOverW += v.oneOverW;
        return *this;
    }

public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec4 clip;
    slib::vec3 worldOverW{};
    float oneOverW = 1.0f;
    bool dirty = false;
};

class Lit {
public:
    Lit() {}

    Lit(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp,
        slib::vec3 _world, bool _dirty, slib::vec3 _worldOverW = {}, float _oneOverW = 1.0f)
        : p_x(px), p_y(py), p_z(pz), normal(n), clip(vp), world(_world), dirty(_dirty),
          worldOverW(_worldOverW), oneOverW(_oneOverW) {}

    Lit operator+(const Lit &v) const {
        return Lit(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal,
                   clip + v.clip, world + v.world, true,
                   worldOverW + v.worldOverW, oneOverW + v.oneOverW);
    }

    Lit operator-(const Lit &v) const {
        return Lit(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal,
                   clip - v.clip, world - v.world, true,
                   worldOverW - v.worldOverW, oneOverW - v.oneOverW);
    }

    Lit operator*(const float &rhs) const {
        return Lit(p_x * rhs, p_y, p_z * rhs, normal * rhs, clip * rhs, world * rhs, true,
                   worldOverW * rhs, oneOverW * rhs);
    }

    Lit &operator+=(const Lit &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        clip += v.clip;
        world += v.world;
        worldOverW += v.worldOverW;
        oneOverW += v.oneOverW;
        return *this;
    }

    Lit &vraster(const Lit &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        worldOverW += v.worldOverW;
        oneOverW += v.oneOverW;
        return *this;
    }

    Lit &hraster(const Lit &v) {
        p_z += v.p_z;
        normal += v.normal;
        worldOverW += v.worldOverW;
        oneOverW += v.oneOverW;
        return *this;
    }

public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 clip;
    slib::vec3 worldOverW{};
    float oneOverW = 1.0f;
    bool dirty = false;
};

class TexturedFlat {
public:
    TexturedFlat() {}

    TexturedFlat(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::zvec2 _tex,
                 slib::vec3 _world, bool _dirty, slib::vec3 _worldOverW = {})
        : p_x(px), p_y(py), p_z(pz), clip(vp), tex(_tex), world(_world), dirty(_dirty),
          worldOverW(_worldOverW) {}

    TexturedFlat operator+(const TexturedFlat &v) const {
        return TexturedFlat(p_x + v.p_x, p_y, p_z + v.p_z, clip + v.clip, tex + v.tex,
                            world + v.world, true, worldOverW + v.worldOverW);
    }

    TexturedFlat operator-(const TexturedFlat &v) const {
        return TexturedFlat(p_x - v.p_x, p_y, p_z - v.p_z, clip - v.clip, tex - v.tex,
                            world - v.world, true, worldOverW - v.worldOverW);
    }

    TexturedFlat operator*(const float &rhs) const {
        return TexturedFlat(p_x * rhs, p_y, p_z * rhs, clip * rhs, tex * rhs, world * rhs, true,
                            worldOverW * rhs);
    }

    TexturedFlat &operator+=(const TexturedFlat &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        clip += v.clip;
        tex += v.tex;
        world += v.world;
        worldOverW += v.worldOverW;
        return *this;
    }

    TexturedFlat &vraster(const TexturedFlat &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        tex += v.tex;
        worldOverW += v.worldOverW;
        return *this;
    }

    TexturedFlat &hraster(const TexturedFlat &v) {
        p_z += v.p_z;
        tex += v.tex;
        worldOverW += v.worldOverW;
        return *this;
    }

public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec4 clip;
    slib::zvec2 tex;
    slib::zvec2 texOverW;
    slib::vec3 worldOverW{};
    bool dirty = false;
};

class TexturedLit {
public:
    TexturedLit() {}

    TexturedLit(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp,
                slib::vec3 _world, slib::zvec2 _tex, bool _dirty, slib::vec3 _worldOverW = {})
        : p_x(px), p_y(py), p_z(pz), normal(n), clip(vp), world(_world), tex(_tex),
          dirty(_dirty), worldOverW(_worldOverW) {}

    TexturedLit operator+(const TexturedLit &v) const {
        return TexturedLit(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal,
                           clip + v.clip, world + v.world, tex + v.tex, true,
                           worldOverW + v.worldOverW);
    }

    TexturedLit operator-(const TexturedLit &v) const {
        return TexturedLit(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal,
                           clip - v.clip, world - v.world, tex - v.tex, true,
                           worldOverW - v.worldOverW);
    }

    TexturedLit operator*(const float &rhs) const {
        return TexturedLit(p_x * rhs, p_y, p_z * rhs, normal * rhs, clip * rhs,
                           world * rhs, tex * rhs, true, worldOverW * rhs);
    }

    TexturedLit &operator+=(const TexturedLit &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        clip += v.clip;
        world += v.world;
        tex += v.tex;
        worldOverW += v.worldOverW;
        return *this;
    }

    TexturedLit &vraster(const TexturedLit &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        tex += v.tex;
        worldOverW += v.worldOverW;
        return *this;
    }

    TexturedLit &hraster(const TexturedLit &v) {
        p_z += v.p_z;
        normal += v.normal;
        tex += v.tex;
        worldOverW += v.worldOverW;
        return *this;
    }

public:
    int32_t p_x;
    int32_t p_y;
    float p_z;
    slib::vec3 world;
    slib::vec3 normal;
    slib::vec4 clip;
    slib::zvec2 tex;
    slib::zvec2 texOverW;
    slib::vec3 worldOverW{};
    bool dirty = false;
};

class Shadow {
public:
    Shadow() {}

    Shadow(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::vec3 _world, bool _dirty)
        : p_x(px), p_y(py), p_z(pz), clip(vp), world(_world), dirty(_dirty) {}

    Shadow operator+(const Shadow &v) const {
        return Shadow(p_x + v.p_x, p_y, p_z + v.p_z, clip + v.clip, world + v.world, true);
    }

    Shadow operator-(const Shadow &v) const {
        return Shadow(p_x - v.p_x, p_y, p_z - v.p_z, clip - v.clip, world - v.world, true);
    }

    Shadow operator*(const float &rhs) const {
        return Shadow(static_cast<int32_t>(p_x * rhs), p_y, p_z * rhs, clip * rhs, world * rhs, true);
    }

    Shadow &operator+=(const Shadow &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        clip += v.clip;
        world += v.world;
        return *this;
    }

    Shadow &vraster(const Shadow &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        return *this;
    }

    Shadow &hraster(const Shadow &v) {
        p_z += v.p_z;
        return *this;
    }

public:
    int32_t p_x = 0;
    int32_t p_y = 0;
    float p_z = 0.0f;
    slib::vec3 world{};
    slib::vec4 clip{};
    slib::vec3 worldOverW{};
    float oneOverW = 1.0f;
    bool dirty = false;
};

} // namespace vertex
