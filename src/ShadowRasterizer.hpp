#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include "slib.hpp"
#include "smath.hpp"
#include "ShadowMap.hpp"
#include "effects/shadowEffect.hpp"
#include "objects/solid.hpp"
#include "slope.hpp"
#include "clipping.hpp"

// Simplified polygon for shadow pass (no material needed)
template<typename Vertex>
struct ShadowPolygon {
    std::vector<Vertex> points;
    slib::vec3 rotatedFaceNormal;

    ShadowPolygon() = default;
    ShadowPolygon(std::vector<Vertex> pts, slib::vec3 normal)
        : points(std::move(pts)), rotatedFaceNormal(normal) {}
};

class ShadowRasterizer {
public:
    using Vertex = ShadowEffect::Vertex;

    ShadowRasterizer() : modelMatrix(smath::identity()) {}

    void renderSolid(Solid& solid, ShadowMap& shadowMap) {
        this->shadowMap = &shadowMap;
        calculateModelMatrix(solid);
        processVertices(solid);
        drawFaces(solid);
    }

private:
    std::vector<Vertex> projectedPoints;
    ShadowMap* shadowMap = nullptr;
    slib::mat4 modelMatrix;
    ShadowEffect effect;

    void calculateModelMatrix(const Solid& solid) {
        slib::mat4 rotate = smath::rotation(
            slib::vec3({solid.position.xAngle, solid.position.yAngle, solid.position.zAngle}));
        slib::mat4 translate = smath::translation(
            slib::vec3({solid.position.x, solid.position.y, solid.position.z}));
        slib::mat4 scale = smath::scale(
            slib::vec3({solid.position.zoom, solid.position.zoom, solid.position.zoom}));
        modelMatrix = translate * rotate * scale;
    }

    void processVertices(Solid& solid) {
        projectedPoints.resize(solid.numVertices);

        std::transform(
            solid.vertexData.begin(),
            solid.vertexData.end(),
            projectedPoints.begin(),
            [&](const auto& vData) {
                return effect.vs(vData, modelMatrix, *shadowMap);
            }
        );
    }

    void drawFaces(Solid& solid) {
        slib::mat4 normalMatrix = smath::rotation(
            slib::vec3({solid.position.xAngle, solid.position.yAngle, solid.position.zAngle}));

        for (int i = 0; i < static_cast<int>(solid.faceData.size()); ++i) {
            const auto& faceDataEntry = solid.faceData[i];

            // Rotate face normal
            slib::vec4 rotatedNormal4 = normalMatrix * slib::vec4(faceDataEntry.faceNormal, 0);
            slib::vec3 rotatedFaceNormal = {rotatedNormal4.x, rotatedNormal4.y, rotatedNormal4.z};

            // Build polygon from projected vertices
            std::vector<Vertex> polyVerts;
            polyVerts.reserve(faceDataEntry.face.vertexIndices.size());
            for (int j : faceDataEntry.face.vertexIndices) {
                polyVerts.push_back(projectedPoints[j]);
            }

            ShadowPolygon<Vertex> poly(std::move(polyVerts), rotatedFaceNormal);

            // Clip against frustum
            auto clippedPoly = clipPolygon(poly);
            if (!clippedPoly.points.empty()) {
                // Project clipped vertices to shadow map space
                for (auto& v : clippedPoly.points) {
                    projectToShadowMap(v);
                }
                drawPolygon(clippedPoly);
            }
        }
    }

    // Project vertex from clip space to shadow map pixel coordinates
    void projectToShadowMap(Vertex& v) {
        if (std::abs(v.ndc.w) < 0.0001f) return;

        float oneOverW = 1.0f / v.ndc.w;
        float ndcX = v.ndc.x * oneOverW;
        float ndcY = v.ndc.y * oneOverW;

        // Map from NDC [-1,1] to shadow map [0, width/height]
        float sx = (ndcX * 0.5f + 0.5f) * shadowMap->width + 0.5f;
        float sy = (ndcY * 0.5f + 0.5f) * shadowMap->height + 0.5f;

        // 16.16 fixed-point for subpixel precision
        constexpr float FP = 65536.0f;
        v.p_x = static_cast<int32_t>(sx * FP);
        v.p_y = static_cast<int32_t>(sy * FP);
        v.p_z = v.ndc.z * oneOverW;
    }

    // Sutherland-Hodgman clipping for shadow polygons (reuses clipping.hpp)
    ShadowPolygon<Vertex> clipPolygon(const ShadowPolygon<Vertex>& poly) {
        std::vector<Vertex> polygon = poly.points;

        for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right, ClipPlane::Bottom,
                                ClipPlane::Top, ClipPlane::Near, ClipPlane::Far}) {
            polygon = ClipAgainstPlane(polygon, plane);
            if (polygon.empty()) {
                return ShadowPolygon<Vertex>({}, poly.rotatedFaceNormal);
            }
        }

        return ShadowPolygon<Vertex>(polygon, poly.rotatedFaceNormal);
    }

    void drawPolygon(ShadowPolygon<Vertex>& polygon) {
        auto begin = std::begin(polygon.points);
        auto end = std::end(polygon.points);

        // Find top-left and bottom-right vertices
        auto cmp_top_left = [](const Vertex& a, const Vertex& b) {
            return std::tie(a.p_y, a.p_x) < std::tie(b.p_y, b.p_x);
        };
        auto [first, last] = std::minmax_element(begin, end, cmp_top_left);

        std::array<decltype(first), 2> cur{first, first};
        auto gety = [&](int side) -> int { return cur[side]->p_y >> 16; };

        int forwards = 1;
        Slope<Vertex> slopes[2]{};

        for (int side = 0, cury = gety(side), nexty[2] = {cury, cury}; cur[side] != last;) {
            auto prev = std::move(cur[side]);

            if (side == forwards) {
                cur[side] = (std::next(prev) == end) ? begin : std::next(prev);
            } else {
                cur[side] = std::prev(prev == begin ? end : prev);
            }

            nexty[side] = gety(side);
            slopes[side] = Slope<Vertex>(*prev, *cur[side], nexty[side] - cury);

            side = (nexty[0] <= nexty[1]) ? 0 : 1;

            for (int limit = nexty[side]; cury < limit; ++cury) {
                drawScanline(cury, slopes[0], slopes[1]);
            }
        }
    }

    void drawScanline(int y, Slope<Vertex>& left, Slope<Vertex>& right) {
        int xStart = left.getx();
        int xEnd = right.getx();

        if (xStart > xEnd) std::swap(xStart, xEnd);

        int dx = xEnd - xStart;

        if (dx > 0) {
            float invDx = 1.0f / dx;
            Vertex vStart = left.get();
            Vertex vStep = (right.get() - vStart) * invDx;

            for (int x = xStart; x < xEnd; ++x) {
                // Only write depth - shadow map acts as z-buffer
                shadowMap->testAndSetDepth(x, y, vStart.p_z);
                vStart.hraster(vStep);
            }
        }

        left.down();
        right.down();
    }
};
