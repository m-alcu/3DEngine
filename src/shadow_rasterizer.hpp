#pragma once
#include <cstdint>
#include <vector>
#include "scene.hpp"
#include "ecs/LightComponent.hpp"
#include "ecs/MeshComponent.hpp"
#include "ecs/ShadowComponent.hpp"
#include "ecs/RenderComponent.hpp"
#include "ecs/TransformComponent.hpp"
#include "ecs/TransformSystem.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "polygon.hpp"
#include "clipping.hpp"
#include "slope.hpp"
#include "edge_walker.hpp"
#include "projection.hpp"
#include "rasterizer_utils.hpp"

template<class Effect>
class ShadowRasterizer {
    public:
        using vertex = typename Effect::Vertex;

        void drawRenderable(TransformComponent& transform,
                            MeshComponent& mesh,
                            LightComponent* lightSrc,
                            ShadowComponent* shadowSrc) {
            transformComponent = &transform;
            meshComponent = &mesh;
            lightSource = lightSrc;
            shadowComponent = shadowSrc;
            screenWidth = shadowComponent->shadowMap->width;
            screenHeight = shadowComponent->shadowMap->height;

            processVertices();
            drawShadowFaces();
        }

    private:
        std::vector<vertex> projectedPoints;
        TransformComponent* transformComponent = nullptr;
        MeshComponent* meshComponent = nullptr;
        LightComponent* lightSource = nullptr;
        ShadowComponent* shadowComponent = nullptr;
        int32_t screenWidth = 0;
        int32_t screenHeight = 0;
        Effect effect;
        Projection<vertex> projection;

        void processVertices() {
            projectedPoints.resize(meshComponent->numVertices);
            const int n = static_cast<int>(meshComponent->vertexData.size());

            #pragma omp parallel for if(n > 1000)
            for (int i = 0; i < n; ++i) {
                projectedPoints[i] = effect.vs(meshComponent->vertexData[i], *transformComponent, shadowComponent);
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
                    effect.ps(x, p_z, *shadowComponent->shadowMap);
                    p_z += p_z_step;
                }
            });
        }
};
