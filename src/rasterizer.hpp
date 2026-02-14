#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "scene.hpp"
#include "ecs/LightComponent.hpp"
#include "ecs/MeshComponent.hpp"
#include "ecs/MaterialComponent.hpp"
#include "ecs/RenderComponent.hpp"
#include "ecs/TransformComponent.hpp"
#include "ecs/TransformSystem.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "polygon.hpp"
#include "clipping.hpp"
#include "slope.hpp"
#include "rasterizer_walker.hpp"
#include "projection.hpp"
#include "rasterizer_utils.hpp"

template<class Effect>
class Rasterizer {
    public:
        using vertex = typename Effect::Vertex;

        void drawRenderable(TransformComponent& transform,
                            MeshComponent& mesh,
                            MaterialComponent& material,
                            Shading shadingMode,
                            Scene* scn) {
            transformComponent = &transform;
            meshComponent = &mesh;
            materialComponent = &material;
            shading = shadingMode;
            scene = scn;
            screenWidth = scene->screen.width;
            screenHeight = scene->screen.height;

            processVertices();
            drawFaces();
        }

    private:
        std::vector<vertex> projectedPoints;
        TransformComponent* transformComponent = nullptr;
        MeshComponent* meshComponent = nullptr;
        MaterialComponent* materialComponent = nullptr;
        Scene* scene = nullptr;
        Shading shading = Shading::Flat;
        int32_t screenWidth = 0;
        int32_t screenHeight = 0;
        Effect effect;
		Projection<vertex> projection;

        void processVertices() {
            projectedPoints.resize(meshComponent->numVertices);
            const int n = static_cast<int>(meshComponent->vertexData.size());

            #pragma omp parallel for if(n > 1000)
            for (int i = 0; i < n; ++i) {
                projectedPoints[i] = effect.vs(meshComponent->vertexData[i], *transformComponent, scene);
                scene->stats.addProcessedVertex();
            }
        }

        void drawFaces() {
            struct FaceDepth {
                int faceIndex;
                float depth;
            };
            std::vector<FaceDepth> visibleFaces;
            visibleFaces.reserve(meshComponent->faceData.size());

            for (int i = 0; i < static_cast<int>(meshComponent->faceData.size()); ++i) {
                const auto& faceDataEntry = meshComponent->faceData[i];
                slib::vec3 normal = TransformSystem::rotateNormal(*transformComponent, faceDataEntry.faceNormal);
                vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                if (shading == Shading::Wireframe || scene->camera.isVisibleFromCamera(p1.world, normal))
                    visibleFaces.push_back({i, p1.p_z});
            }

            if (scene->depthSortEnabled) {
                std::sort(visibleFaces.begin(), visibleFaces.end(),
                    [](const FaceDepth& a, const FaceDepth& b) { return a.depth < b.depth; });
            }

            //#pragma omp parallel for
            //This will wait until we develop a tile-based rasterizer
            for (const auto& fd : visibleFaces) {
                const auto& faceDataEntry = meshComponent->faceData[fd.faceIndex];
                slib::vec3 normal = TransformSystem::rotateNormal(*transformComponent, faceDataEntry.faceNormal);

                Polygon<vertex> poly(
                    collectPolyVerts(projectedPoints, faceDataEntry),
                    normal,
                    materialComponent->materials.at(faceDataEntry.face.materialKey)
                );
                clipAndDraw(poly);
            }
        }

        inline void clipAndDraw(Polygon<vertex>& poly) {
            auto clippedPoly = ClipCullPolygon(poly);
            if (!clippedPoly.points.empty()) {
                drawPolygon(clippedPoly);
                scene->stats.addDrawCall();
            }
        }        

        // Unified polygon drawing for both regular and shadow rendering
        void drawPolygon(Polygon<vertex>& polygon) {
            effect.gs(polygon, screenWidth, screenHeight);
            scene->stats.addPoly();
            uint32_t* pixels = static_cast<uint32_t*>(scene->pixels);
            if (shading == Shading::Wireframe) {
                polygon.drawWireframe(WHITE_COLOR, pixels, screenWidth, screenHeight, scene->zBuffer.get());
            } else {
                rasterizeFilledPolygon(polygon, pixels);
            }
        }

        void rasterizeFilledPolygon(Polygon<vertex>& polygon, uint32_t* pixels) {
            EdgeWalker<vertex> walker(polygon.points, screenWidth);
            walker.walk([&](int xStart, int xEnd, int dx, Slope<vertex>& left, Slope<vertex>& right) {
                float invDx = 1.0f / dx;
                vertex vStart = left.get();
                vertex vStep = (right.get() - vStart) * invDx;

                for (int x = xStart; x < xEnd; ++x) {
                    if (scene->zBuffer->TestAndSet(x, vStart.p_z)) {
                        pixels[x] = effect.ps(vStart, *scene, polygon);
                        scene->stats.addPixel();
                    }
                    vStart.hraster(vStep);
                }
            });
        }

    };
