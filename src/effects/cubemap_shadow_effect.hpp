#pragma once
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/shadow_component.hpp"
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "../shadow_map.hpp"
#include "../cube_shadow_map.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include <cstdio>

class Scene;

// Effect for rendering to cubemap shadow faces
class CubemapShadowEffect {
public:
    static constexpr bool is_shadow_effect = true;

    // Current face being rendered
    CubeShadowFace currentFace = CubeShadowFace::POSITIVE_X;

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
        bool broken = false;
    };

    class VertexShader {
    public:
        CubeShadowFace face;

        VertexShader(CubeShadowFace f = CubeShadowFace::POSITIVE_X) : face(f) {}

        Vertex operator()(const VertexData& vData,
                          const TransformComponent& transform,
                          const ShadowComponent* shadowSource) const {
            Vertex vertex;
            vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);

            // Use the appropriate face's view matrix
            const auto& cubeShadowMap = shadowSource->shadowMap->cubeShadowMap;
            int faceIdx = static_cast<int>(face);
            vertex.ndc = slib::vec4(vertex.world, 1) * cubeShadowMap->lightSpaceMatrices[faceIdx];
            
            Projection<Vertex>::view(cubeShadowMap->faceSize, cubeShadowMap->faceSize, vertex, true);

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
        CubeShadowFace face;

        PixelShader(CubeShadowFace f = CubeShadowFace::POSITIVE_X) : face(f) {}

        void operator()(int x, float p_z, CubeShadowMap& cubeShadowMap) const {
            cubeShadowMap.testAndSetDepth(face, x, p_z);
        }
    };

public:
    VertexShader vs;
    GeometryShader gs;
    PixelShader ps;

    CubemapShadowEffect() : vs(currentFace), ps(currentFace) {}

    void setFace(CubeShadowFace face) {
        currentFace = face;
        vs.face = face;
        ps.face = face;
    }
};
