#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <type_traits>
#include "scene.hpp"
#include "ecs/LightComponent.hpp"
#include "ecs/MeshComponent.hpp"
#include "ecs/RenderComponent.hpp"
#include "ecs/TransformComponent.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "polygon.hpp"
#include "clipping.hpp"
#include "slope.hpp"
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
                            Shading shadingMode,
                            Scene* scn,
                            LightComponent* lightSrc = nullptr) {
            transformComponent = &transform;
            meshComponent = &mesh;
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

        inline slib::vec3 getRotatedNormal(const FaceData& faceDataEntry) const {
            slib::vec4 rotated = transformComponent->normalMatrix * slib::vec4(faceDataEntry.faceNormal, 0);
            return {rotated.x, rotated.y, rotated.z};
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

            //#pragma omp parallel
            {
                std::vector<FaceDepth> localVisible;
                //#pragma omp for nowait
                for (int i = 0; i < static_cast<int>(meshComponent->faceData.size()); ++i) {
                    const auto& faceDataEntry = meshComponent->faceData[i];
                    slib::vec3 normal = getRotatedNormal(faceDataEntry);
                    vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                    if (shading == Shading::Wireframe || isFaceVisibleFromCamera(p1.world, normal))
                        localVisible.push_back({i, p1.p_z});
                }
                //#pragma omp critical
                visibleFaces.insert(visibleFaces.end(), localVisible.begin(), localVisible.end());
            }

            if (scene->depthSortEnabled) {
                std::sort(visibleFaces.begin(), visibleFaces.end(),
                    [](const FaceDepth& a, const FaceDepth& b) { return a.depth < b.depth; });
            }

            //#pragma omp parallel for
            //This will wait until we develop a tile-based rasterizer
            for (const auto& fd : visibleFaces) {
                const auto& faceDataEntry = meshComponent->faceData[fd.faceIndex];
                slib::vec3 normal = getRotatedNormal(faceDataEntry);

                Polygon<vertex> poly(
                    collectPolyVerts(faceDataEntry),
                    normal,
                    meshComponent->materials.at(faceDataEntry.face.materialKey)
                );
                clipAndDraw(poly);
            }
        }

        void drawShadowFaces()
        {
            for (const auto &faceDataEntry : meshComponent->faceData)
            {
                slib::vec3 normal = getRotatedNormal(faceDataEntry);
                vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                if (isFaceVisibleFromLight(p1.world, normal))
                {
                    Polygon<vertex> poly(collectPolyVerts(faceDataEntry), normal);
                    clipAndDraw(poly);
                }
            }
        }

        bool isFaceVisibleFromCamera(const slib::vec3& world, const slib::vec3& faceNormal) const {
            slib::vec3 viewDir = scene->camera.pos - world;
            float dotResult = smath::dot(faceNormal, smath::normalize(viewDir));
            return dotResult > 0.0f;
        }

        bool isFaceVisibleFromLight(const slib::vec3& world, const slib::vec3& faceNormal) const {
            slib::vec3 normalizedLightDir = lightSource->light.getDirection(world);
            float dotResult = smath::dot(faceNormal, normalizedLightDir);
            return dotResult > 0.0f;
        }

        // Unified polygon drawing for both regular and shadow rendering
        void drawPolygon(Polygon<vertex>& polygon) {
            uint32_t* pixels = static_cast<uint32_t*>(scene->pixels);
            effect.gs(polygon, screenWidth, screenHeight, *scene);
            if constexpr (!isShadowEffect) {
                scene->stats.addPoly();
            }

            if constexpr (!isShadowEffect) {
                if (shading == Shading::Wireframe) {
                    polygon.drawWireframe(WHITE_COLOR, pixels, screenWidth, screenHeight, scene->zBuffer.get());
                    return;
                }
            }

            auto begin = std::begin(polygon.points), end = std::end(polygon.points);

            auto cmp_top_left = [&](const vertex& a, const vertex& b) {
                return std::tie(a.p_y, a.p_x) < std::tie(b.p_y, b.p_x);
            };
            auto [first, last] = std::minmax_element(begin, end, cmp_top_left);

            std::array<decltype(first), 2> cur{ first, first };
            auto gety = [&](int side) -> int { return cur[side]->p_y >> 16; };

            int forwards = 0;
            Slope<vertex> slopes[2] {};

            for(int side = 0, cury = gety(side), nexty[2] = {cury,cury}, hy = cury * screenWidth; cur[side] != last; )
            {
                auto prev = std::move(cur[side]);

                if(side == forwards) cur[side] = (std::next(prev) == end) ? begin : std::next(prev);
                else                 cur[side] = std::prev(prev == begin ? end : prev);

                nexty[side]  = gety(side);
                slopes[side] = Slope<vertex>(*prev, *cur[side], nexty[side] - cury);

                side = (nexty[0] <= nexty[1]) ? 0 : 1;
                for(int limit = nexty[side]; cury < limit; ++cury, hy+= screenWidth) {
                    drawScanline(hy, slopes[0], slopes[1], polygon, pixels);
                }
            }
        }

        inline void drawScanline(const int& hy, Slope<vertex>& left, Slope<vertex>& right, Polygon<vertex>& polygon, uint32_t* pixels) {
            int xStart = left.getx() + hy;
            int xEnd = right.getx() + hy;
            int dx = xEnd - xStart;


            if constexpr (isShadowEffect) {
                if (dx > 0) {
                    float invDx = 1.0f / dx;
                    float p_z = left.get().p_z;
                    float p_z_step = (right.get().p_z - p_z) * invDx;
                    for (int x = xStart; x < xEnd; ++x) {
                        effect.ps(x, p_z, *lightSource->shadowMap);
                        p_z += p_z_step;
                    }
                }
            } else {
                if (dx > 0) {
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
            }

            left.down();
            right.down();
        }

    };
