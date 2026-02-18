#pragma once
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/shadow_component.hpp"
#include "../ecs/mesh_component.hpp"
#include "../ecs/transform_component.hpp"
#include "../shadow_map.hpp"
#include "../polygon.hpp"
#include "../projection.hpp"
#include "vertex_types.hpp"

// Minimal effect for shadow pass - only tracks position/depth, no shading
// Supports both single-face (directional/spot) and multi-face (cubemap) shadows
class ShadowEffect {
public:
    using Vertex = vertex::Shadow;

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
