#pragma once
#include <cstdint>
#include <vector>
#include "scene.hpp"
#include "ecs/light_component.hpp"
#include "ecs/mesh_component.hpp"
#include "ecs/shadow_component.hpp"
#include "ecs/render_component.hpp"
#include "ecs/transform_component.hpp"
#include "ecs/transform_system.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "polygon.hpp"
#include "clipping.hpp"
#include "rasterizer_slope.hpp"
#include "rasterizer_walker.hpp"
#include "projection.hpp"
#include "rasterizer_utils.hpp"

template<class Effect>
class ShadowRasterizer {
    public:
        using vertex = typename Effect::Vertex;

        void drawRenderable(const TransformComponent& transform,
                            const MeshComponent& mesh,
                            const LightComponent& light,
                            const ShadowComponent& shadow,
                            int faceIndex = 0) {
            transformComponent = &transform;
            meshComponent = &mesh;
            lightSource = &light;
            shadowComponent = &shadow;
            screenWidth = shadowComponent->shadowMap->getFaceWidth();
            screenHeight = shadowComponent->shadowMap->getFaceHeight();
            faceIdx = faceIndex;
            processVertices();
            drawShadowFaces();
        }

    private:
        std::vector<vertex> projectedPoints;
        const TransformComponent* transformComponent = nullptr;
        const MeshComponent* meshComponent = nullptr;
        const LightComponent* lightSource = nullptr;
        const ShadowComponent* shadowComponent = nullptr;
        int32_t screenWidth = 0;
        int32_t screenHeight = 0;
        Effect effect;
        Projection<vertex> projection;
        int faceIdx = 0;

        void processVertices() {
            projectedPoints.resize(meshComponent->numVertices);
            const int n = static_cast<int>(meshComponent->vertexData.size());

            #pragma omp parallel for if(n > 1000)
            for (int i = 0; i < n; ++i) {
                projectedPoints[i] = effect.vs(meshComponent->vertexData[i], *transformComponent, shadowComponent, faceIdx);
            }
        }

        void drawShadowFaces() {
            for (const auto &faceDataEntry : meshComponent->faceData) {
                slib::vec3 normal = TransformSystem::rotateNormal(*transformComponent, faceDataEntry.faceNormal);
                vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                if (lightSource->light.isVisibleFromLight(p1.world, normal)) {
                    Polygon<vertex> poly(collectPolyVerts(projectedPoints, faceDataEntry), normal);
                    clipAndDraw(poly);
                }
            }
        }

        inline void clipAndDraw(Polygon<vertex>& poly) {
            auto clippedPoly = ClipCullPolygon(poly);
            if (!clippedPoly.points.empty()) {
                shadowComponent->shadowMap->clearFaceIfDirty(faceIdx);
                drawPolygon(clippedPoly);
            }
        }

        void drawPolygon(Polygon<vertex>& polygon) {
            effect.gs(polygon, screenWidth, screenHeight);
            rasterizeFilledPolygon(polygon);
        }

        void rasterizeFilledPolygon(Polygon<vertex>& polygon) {
            EdgeWalker<vertex> walker(polygon.points, screenWidth);
            walker.walk([&](int xStart, int xEnd, int dx, Slope<vertex>& left, Slope<vertex>& right) {
                float invDx = 1.0f / dx;
                float p_z = left.get().p_z;
                float p_z_step = (right.get().p_z - p_z) * invDx;
                for (int x = xStart; x < xEnd; ++x) {
                    effect.ps(x, p_z, *shadowComponent->shadowMap, faceIdx);
                    p_z += p_z_step;
                }
            });
        }
};
