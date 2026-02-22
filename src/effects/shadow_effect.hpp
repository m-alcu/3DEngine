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
#include "geometry_shaders.hpp"

// Minimal effect for shadow pass - only tracks position/depth, no shading
// Supports both single-face (directional/spot) and multi-face (cubemap) shadows
class ShadowEffect {
public:
    using Vertex = vertex::Shadow;
    using GeometryShader = vertex::ViewGeometryShader<Vertex>;

    class VertexShader {
    public:

        Vertex operator()(const VertexData& vData,
                          const TransformComponent& transform,
                          const ShadowComponent& shadow,
                          int faceIdx) const {
            Vertex vertex;
            vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);

            const auto& shadowMap = shadow.shadowMap;
            vertex.ndc = slib::vec4(vertex.world, 1) * shadowMap->getLightSpaceMatrix(faceIdx);
            Projection<Vertex>::view(shadowMap->getFaceWidth(), shadowMap->getFaceHeight(), vertex, true);

            return vertex;
        }
    };

    class PixelShader {
    public:
        void operator()(int x, float p_z, ShadowMap& shadowMap, int faceIdx) const {
            shadowMap.testAndSetDepth(faceIdx, x, p_z);
        }
    };

public:
    VertexShader vs;
    GeometryShader gs;
    PixelShader ps;
};
