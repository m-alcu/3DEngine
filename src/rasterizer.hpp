#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <type_traits>
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
#include "edge_walker.hpp"
#include "projection.hpp"

// Trait to detect shadow effects
template<typename T, typename = void>
struct is_shadow_effect : std::false_type {};

template<typename T>
struct is_shadow_effect<T, std::void_t<decltype(T::is_shadow_effect)>>
    : std::bool_constant<T::is_shadow_effect> {};

template<typename T>
inline constexpr bool is_shadow_effect_v = is_shadow_effect<T>::value;

template<class Effect>
class Rasterizer {
    public:
        using vertex = typename Effect::Vertex;
        static constexpr bool isShadowEffect = is_shadow_effect_v<Effect>;

        void drawRenderable(TransformComponent& transform,
                            MeshComponent& mesh,
                            MaterialComponent& material,
                            Shading shadingMode,
                            Scene* scn,
                            LightComponent* lightSrc = nullptr) {
            transformComponent = &transform;
            meshComponent = &mesh;
            materialComponent = &material;
            shading = shadingMode;
            scene = scn;
            lightSource = lightSrc;
            if constexpr (isShadowEffect) {
                screenWidth = lightSource->shadowMap->width;
                screenHeight = lightSource->shadowMap->height;
            } else {
                screenWidth = scene->screen.width;
                screenHeight = scene->screen.height;
            }

            processVertices();

            if constexpr (isShadowEffect) {
                drawShadowFaces();
            } else {
                drawFaces();
            }
        }

    private:
        std::vector<vertex> projectedPoints;
        TransformComponent* transformComponent = nullptr;
        MeshComponent* meshComponent = nullptr;
        MaterialComponent* materialComponent = nullptr;
        Scene* scene = nullptr;
        LightComponent* lightSource = nullptr;
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
                if constexpr (isShadowEffect) {
                    projectedPoints[i] = effect.vs(meshComponent->vertexData[i], *transformComponent, scene, lightSource);
                } else {
                    projectedPoints[i] = effect.vs(meshComponent->vertexData[i], *transformComponent, scene);
                    scene->stats.addProcessedVertex();
                }
            }
        }

        inline std::vector<vertex> collectPolyVerts(const FaceData& faceDataEntry) const {
            std::vector<vertex> polyVerts;
            polyVerts.reserve(faceDataEntry.face.vertexIndices.size());
            for (int j : faceDataEntry.face.vertexIndices)
                polyVerts.push_back(projectedPoints[j]);
            return polyVerts;
        }

        inline void clipAndDraw(Polygon<vertex>& poly) {
            auto clippedPoly = ClipCullPolygon(poly);
            if (!clippedPoly.points.empty()) {
                drawPolygon(clippedPoly);
                scene->stats.addDrawCall();
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
                    collectPolyVerts(faceDataEntry),
                    normal,
                    materialComponent->materials.at(faceDataEntry.face.materialKey)
                );
                clipAndDraw(poly);
            }
        }

        void drawShadowFaces()
        {
            for (const auto &faceDataEntry : meshComponent->faceData)
            {
                slib::vec3 normal = TransformSystem::rotateNormal(*transformComponent, faceDataEntry.faceNormal);
                vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                if (lightSource->light.isVisibleFromLight(p1.world, normal))
                {
                    Polygon<vertex> poly(collectPolyVerts(faceDataEntry), normal);
                    clipAndDraw(poly);
                }
            }
        }

        // Unified polygon drawing for both regular and shadow rendering
        void drawPolygon(Polygon<vertex>& polygon) {
            uint32_t* pixels = beginPolygonDraw(polygon);
            if (drawWireframeIfNeeded(polygon, pixels)) {
                return;
            }
            rasterizeFilledPolygon(polygon, pixels);
        }

        inline uint32_t* beginPolygonDraw(Polygon<vertex>& polygon) {
            effect.gs(polygon, screenWidth, screenHeight, *scene);
            if constexpr (!isShadowEffect) {
                scene->stats.addPoly();
            }
            return static_cast<uint32_t*>(scene->pixels);
        }

        inline bool drawWireframeIfNeeded(Polygon<vertex>& polygon, uint32_t* pixels) const {
            if constexpr (!isShadowEffect) {
                if (shading == Shading::Wireframe) {
                    polygon.drawWireframe(WHITE_COLOR, pixels, screenWidth, screenHeight, scene->zBuffer.get());
                    return true;
                }
            }
            return false;
        }

        void rasterizeFilledPolygon(Polygon<vertex>& polygon, uint32_t* pixels) {
            EdgeWalker<vertex> walker(polygon.points, screenWidth);
            walker.walk([&](int xStart, int xEnd, int dx, Slope<vertex>& left, Slope<vertex>& right) {
                if constexpr (isShadowEffect) {
                    float invDx = 1.0f / dx;
                    float p_z = left.get().p_z;
                    float p_z_step = (right.get().p_z - p_z) * invDx;
                    for (int x = xStart; x < xEnd; ++x) {
                        effect.ps(x, p_z, *lightSource->shadowMap);
                        p_z += p_z_step;
                    }
                } else {
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
                }
            });
        }

    };
