#pragma once
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/shadow_component.hpp"
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "../shadow_map.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"

class Scene;

// Minimal effect for shadow pass - only tracks position/depth, no shading
// Supports both single-face (directional/spot) and multi-face (cubemap) shadows
class ShadowEffect {
public:
    static constexpr bool is_shadow_effect = true;

    class Vertex {
    public:
        Vertex() {}

        Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::vec3 _world, bool _dirty)
            : p_x(px), p_y(py), p_z(pz), ndc(vp), world(_world), dirty(_dirty) {}

        Vertex operator+(const Vertex& v) const {
            return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, ndc + v.ndc, world + v.world, true);
        }

        Vertex operator-(const Vertex& v) const {
            return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, ndc - v.ndc, world - v.world, true);
        }

        Vertex operator*(const float& rhs) const {
            return Vertex(
                static_cast<int32_t>(p_x * rhs), p_y,
                p_z * rhs, ndc * rhs, world * rhs, true);
        }

        Vertex& operator+=(const Vertex& v) {
            p_x += v.p_x;
            p_z += v.p_z;
            ndc += v.ndc;
            world += v.world;
            return *this;
        }

        Vertex& vraster(const Vertex& v) {
            p_x += v.p_x;
            p_z += v.p_z;
            return *this;
        }

        Vertex& hraster(const Vertex& v) {
            p_z += v.p_z;
            return *this;
        }

    public:
        int32_t p_x = 0;
        int32_t p_y = 0;
        float p_z = 0.0f;
        slib::vec3 world{};
        slib::vec4 ndc{};
        bool dirty = false;
    };

    class VertexShader {
    public:
        int faceIdx = 0;

        Vertex operator()(const VertexData& vData,
                          const TransformComponent& transform,
                          const ShadowComponent* shadowSource) const {
            Vertex vertex;
            vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);

            const auto& shadowMap = shadowSource->shadowMap;
            vertex.ndc = slib::vec4(vertex.world, 1) * shadowMap->getLightSpaceMatrix(faceIdx);
            Projection<Vertex>::view(shadowMap->getFaceWidth(), shadowMap->getFaceHeight(), vertex, true);

            return vertex;
        }
    };

    class GeometryShader {
    public:
        void operator()(Polygon<Vertex>& poly, int32_t width, int32_t height) const {
            for (auto& point : poly.points) {
                Projection<Vertex>::view(width, height, point, false);
            }
        }
    };

    class PixelShader {
    public:
        int faceIdx = 0;

        void operator()(int x, float p_z, ShadowMap& shadowMap) const {
            shadowMap.testAndSetDepth(faceIdx, x, p_z);
        }
    };

    void setFace(int face) {
        vs.faceIdx = face;
        ps.faceIdx = face;
    }

public:
    VertexShader vs;
    GeometryShader gs;
    PixelShader ps;
};
