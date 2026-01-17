#pragma once
#include "../slib.hpp"
#include "../smath.hpp"
#include "../objects/solid.hpp"
#include "../ShadowMap.hpp"
#include "../polygon.hpp"

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
                          const slib::mat4& modelMatrix,
                          const slib::mat4& /*normalMatrix*/,
                          const Scene* /*scene*/,
                          const ShadowMap* shadowMap) const {
            Vertex vertex;

            // Transform to world space
            vertex.world = modelMatrix * slib::vec4(vData.vertex, 1);

            // Transform to light clip space
            vertex.ndc = slib::vec4(vertex.world, 1) * shadowMap->lightSpaceMatrix;

            return vertex;
        }
    };

    class GeometryShader {
    public:
        void operator()(Polygon<Vertex>& poly, const ShadowMap& shadowMap) const {
            // Project clipped vertices to shadow map space
            for (auto& v : poly.points) {
                projectToShadowMap(v, shadowMap);
            }
        }

    private:
        void projectToShadowMap(Vertex& v, const ShadowMap& shadowMap) const {
            if (std::abs(v.ndc.w) < 0.0001f) return;

            float oneOverW = 1.0f / v.ndc.w;
            float ndcX = v.ndc.x * oneOverW;
            float ndcY = v.ndc.y * oneOverW;

            // Map from NDC [-1,1] to shadow map [0, width/height]
            float sx = (ndcX * 0.5f + 0.5f) * shadowMap.width + 0.5f;
            float sy = (ndcY * 0.5f + 0.5f) * shadowMap.height + 0.5f;

            // 16.16 fixed-point for subpixel precision
            constexpr float FP = 65536.0f;
            v.p_x = static_cast<int32_t>(sx * FP);
            v.p_y = static_cast<int32_t>(sy * FP);
            v.p_z = v.ndc.z * oneOverW;
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
