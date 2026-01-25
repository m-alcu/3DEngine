#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <type_traits>
#include "scene.hpp"
#include "objects/solid.hpp"
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

        void drawRenderable(Solid* sol, Scene* scn, ShadowMap* map = nullptr) {
            solid = sol;
            scene = scn;
            shadowMap = map;
            if constexpr (isShadowEffect) {
                screenWidth = shadowMap->width;
                screenHeight = shadowMap->height;
            } else {
                screenWidth = scene->screen.width;
                screenHeight = scene->screen.height;
            }

            processVertices();
            drawFaces();
        }

    private:
        std::vector<vertex> projectedPoints;
        Solid* solid = nullptr;
        Scene* scene = nullptr;
        int32_t screenWidth = 0;
        int32_t screenHeight = 0;
        ShadowMap* shadowMap = nullptr;
        Effect effect;
		Projection<vertex> projection;

        void processVertices() {
            projectedPoints.resize(solid->numVertices);

            std::transform(
                solid->vertexData.begin(),
                solid->vertexData.end(),
                projectedPoints.begin(),
                [&](const auto& vData) {
                    if constexpr (isShadowEffect) {
                        return effect.vs(vData, solid, scene, shadowMap);
                    } else {
                        return effect.vs(vData, solid, scene);
                    }
                }
            );
        }

        void drawFaces() {
            if constexpr (isShadowEffect) {
                // Shadow pass: no sorting needed, just render all visible faces
                for (int i = 0; i < static_cast<int>(solid->faceData.size()); ++i) {
                    const auto& faceDataEntry = solid->faceData[i];
                    slib::vec3 rotatedFaceNormal{};
                    rotatedFaceNormal = solid->normalMatrix * slib::vec4(faceDataEntry.faceNormal, 0);
                    vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                    if (isFaceVisibleFromLight(p1.world, rotatedFaceNormal)) {
                        std::vector<vertex> polyVerts;
                        polyVerts.reserve(faceDataEntry.face.vertexIndices.size());
                        for (int j : faceDataEntry.face.vertexIndices)
                            polyVerts.push_back(projectedPoints[j]);

                        Polygon<vertex> poly(std::move(polyVerts), rotatedFaceNormal);
                        auto clippedPoly = ClipCullPolygon(poly);
                        if (!clippedPoly.points.empty()) {
                            drawPolygon(clippedPoly);
                        }
                    }
                }
            } else {
                // Regular rendering: collect visible faces and sort front-to-back for early Z-rejection
                struct FaceDepth {
                    int faceIndex;
                    float depth;
                };
                std::vector<FaceDepth> visibleFaces;
                visibleFaces.reserve(solid->faceData.size());

                // First pass: collect visible faces with their depths
                for (int i = 0; i < static_cast<int>(solid->faceData.size()); ++i) {
                    const auto& faceDataEntry = solid->faceData[i];
                    slib::vec3 rotatedFaceNormal{};
                    rotatedFaceNormal = solid->normalMatrix * slib::vec4(faceDataEntry.faceNormal, 0);
                    vertex p1 = projectedPoints[faceDataEntry.face.vertexIndices[0]];

                    bool shouldRender = solid->shading == Shading::Wireframe ||
                                        isFaceVisibleFromCamera(p1.world, rotatedFaceNormal);

                    if (shouldRender) {
                        // Compute average depth of face vertices (in view space, smaller = closer)
                        float avgDepth = 0.0f;
                        for (int j : faceDataEntry.face.vertexIndices) {
                            avgDepth += projectedPoints[j].p_z;
                        }
                        avgDepth /= static_cast<float>(faceDataEntry.face.vertexIndices.size());

                        visibleFaces.push_back({i, avgDepth});
                    }
                }

                // Sort front-to-back (smaller depth = closer to camera = render first)
                std::sort(visibleFaces.begin(), visibleFaces.end(),
                    [](const FaceDepth& a, const FaceDepth& b) {
                        return a.depth < b.depth;
                    });

                // Second pass: render in sorted order
                for (const auto& fd : visibleFaces) {
                    const auto& faceDataEntry = solid->faceData[fd.faceIndex];
                    slib::vec3 rotatedFaceNormal{};
                    rotatedFaceNormal = solid->normalMatrix * slib::vec4(faceDataEntry.faceNormal, 0);

                    std::vector<vertex> polyVerts;
                    polyVerts.reserve(faceDataEntry.face.vertexIndices.size());
                    for (int j : faceDataEntry.face.vertexIndices)
                        polyVerts.push_back(projectedPoints[j]);

                    Polygon<vertex> poly(
                        std::move(polyVerts),
                        faceDataEntry.face,
                        rotatedFaceNormal,
                        solid->materials.at(faceDataEntry.face.materialKey)
                    );

                    auto clippedPoly = ClipCullPolygon(poly);
                    if (!clippedPoly.points.empty()) {
                        drawPolygon(clippedPoly);
                    }
                }
            }
        }

        bool isFaceVisibleFromCamera(const slib::vec3& world, const slib::vec3& faceNormal) {
            slib::vec3 viewDir = scene->camera.pos - world;
            viewDir = smath::normalize(viewDir);
            float dotResult = smath::dot(faceNormal, viewDir);
            return dotResult > 0.0f;
        }

        bool isFaceVisibleFromLight(const slib::vec3& world, const slib::vec3& faceNormal) {
            // Calculate direction from surface to light
            slib::vec3 lightDir;
            if (scene->light.type == LightType::Directional) {
                // For directional lights, direction is constant
                lightDir = scene->light.direction;
            } else {
                // For point/spot lights, calculate direction from surface to light position
                lightDir = scene->light.position - world;
            }
            lightDir = smath::normalize(lightDir);

            // Face is visible from light if it's pointing toward the light
            float dotResult = smath::dot(faceNormal, lightDir);
            return dotResult > 0.0f;
        }

        // Unified polygon drawing for both regular and shadow rendering
        void drawPolygon(Polygon<vertex>& polygon) {
            uint32_t* pixels = static_cast<uint32_t*>(scene->pixels);
            effect.gs(polygon, screenWidth, screenHeight, *scene);

            if constexpr (!isShadowEffect) {
                if (solid->shading == Shading::Wireframe) {
                    drawWireframePolygon(polygon, 0xffffffff, pixels);
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
                        effect.ps(x, p_z, *shadowMap);
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
                        }
                        vStart.hraster(vStep);
                    }
                }
            }

            left.down();
            right.down();
        }

        void drawWireframePolygon(Polygon<vertex> polygon, uint32_t color, uint32_t* pixels) {
            const size_t n = polygon.points.size();
            for (size_t i = 0, j = n - 1; i < n; j = i++) {
                auto& v0 = polygon.points[j];
                auto& v1 = polygon.points[i];

                drawBresenhamLine(v0.p_x >> 16, v0.p_y >> 16, v1.p_x >> 16, v1.p_y >> 16, pixels, color);
            }
        }

        void drawBresenhamLine(int x0, int y0, int x1, int y1, uint32_t* pixels, uint32_t color) {
            int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
            int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
            int err = dx + dy, e2;

            while (true) {
                if (x0 >= 0 && x0 < screenWidth && y0 >= 0 && y0 < screenHeight)
                    pixels[y0 * screenWidth + x0] = color;

                if (x0 == x1 && y0 == y1) break;
                e2 = 2 * err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
            }
        }

    };
