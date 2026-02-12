#pragma once
#include "../slib.hpp"
#include "../smath.hpp"
#include "../objects/solid.hpp"
#include "../ShadowMap.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"

class Scene;

// Minimal effect for shadow pass - only tracks position/depth, no shading
class ShadowEffect {
public:
    // Tag to identify this as a shadow effect
    static constexpr bool is_shadow_effect = true;

    class Vertex {
    public:
        Vertex() {}

        Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::vec3 _world, bool _broken)
            : p_x(px), p_y(py), p_z(pz), ndc(vp), world(_world), broken(_broken) {}

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

        // Vertical raster step (moving down scanlines)
        Vertex& vraster(const Vertex& v) {
            p_x += v.p_x;
            p_z += v.p_z;
            return *this;
        }

        // Horizontal raster step (moving across scanline)
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
        bool broken = false;
    };

    class VertexShader {
    public:
        Vertex operator()(const VertexData& vData,
                          const Solid* solid,
                          const Scene* /*scene*/,
                          const Solid* lightSource) const {
            Vertex vertex;
            // Transform to world space
            vertex.world = solid->transform.modelMatrix * slib::vec4(vData.vertex, 1);

            // Transform to light clip space
            const auto& shadowMap = lightSource->lightComponent->shadowMap;
            vertex.ndc = slib::vec4(vertex.world, 1) * shadowMap->lightSpaceMatrix;
            Projection<Vertex>::view(shadowMap->width, shadowMap->height, vertex, true);

            return vertex;
        }
    };

    class GeometryShader {
    public:
        void operator()(Polygon<Vertex>& poly, int32_t width, int32_t height, const Scene &/*scene*/) const {
            // Project clipped vertices to shadow map space
            for (auto& point : poly.points) {
                Projection<Vertex>::view(width, height, point, false);
            }
        }
    };

    class PixelShader {
    public:
        void operator()(int x, float p_z, ShadowMap& shadowMap) const {
            shadowMap.testAndSetDepth(x, p_z);
        }
    };

public:
    VertexShader vs;
    GeometryShader gs;
    PixelShader ps;
};
