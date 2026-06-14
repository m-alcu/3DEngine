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
        // Stage 1: model -> world space (independent of which cubemap face).
        Vertex world(const VertexData& vData, const TransformComponent& transform) const {
            Vertex vertex;
            vertex.world = transform.modelMatrix * slib::vec4(vData.vertex, 1);
            return vertex;
        }

        // Stage 2: world -> light clip + screen space for a given face.
        void project(Vertex& vertex, const ShadowMap& shadowMap, int faceIdx) const {
            vertex.clip = slib::vec4(vertex.world, 1) * shadowMap.getLightSpaceMatrix(faceIdx);
            vertex.dirty = true;
            Projection<Vertex>::view(shadowMap.getFaceWidth(), shadowMap.getFaceHeight(), vertex);
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
