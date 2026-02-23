#pragma once
#include <vector>
#include "polygon.hpp"
#include "scene.hpp" // If needed for material, etc.

enum class ClipPlane {
    Left, Right, Bottom, Top, Near, Far
};

/*
Clipping is done using the Sutherland-Hodgman algorithm (1974) in clip space.
The Sutherland-Hodgman algorithm is a polygon clipping algorithm that clips a polygon against a convex clipping region.
The algorithm works by iterating through each edge of the polygon and checking if the vertices are inside or outside the clipping plane.
If a vertex is inside, it is added to the output polygon. If a vertex is outside, the algorithm checks if the previous vertex was inside. If it was, the edge between the two vertices is clipped and the intersection point is added to the output polygon.
The algorithm continues until all edges have been processed.
https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
*/


template<typename Vertex>
Polygon<Vertex> ClipCullPolygon(const Polygon<Vertex>& t) {
    // Double-buffered clipping with per-plane all-inside skip
    std::vector<Vertex> bufA;
    std::vector<Vertex> bufB;
    bool copied = false;

    for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right, ClipPlane::Bottom,
        ClipPlane::Top, ClipPlane::Near, ClipPlane::Far}) {

        // Check if all vertices are inside this plane
        const std::vector<Vertex>& input = copied ? bufA : t.points;
        bool allInside = true;
        for (const auto& v : input) {
            if (!IsInside(v, plane)) {
                allInside = false;
                break;
            }
        }
        if (allInside) continue;

        // Need to clip — lazy-copy on first plane that needs it
        if (!copied) {
            bufA = t.points;
            bufB.reserve(bufA.size() + 6);
            copied = true;
        }

        ClipAgainstPlane(bufA, bufB, plane);
        if (bufB.empty()) {
            return Polygon<Vertex>(std::move(bufB), t.rotatedFaceNormal, *t.material);
        }
        std::swap(bufA, bufB);
    }

    // No plane needed clipping — return original unchanged
    if (!copied) {
        return t;
    }

    return Polygon<Vertex>(std::move(bufA), t.rotatedFaceNormal, *t.material);
}

template<typename Vertex>
void ClipAgainstPlane(const std::vector<Vertex>& poly, std::vector<Vertex>& output, ClipPlane plane) {
    output.clear();
    const size_t n = poly.size();
    if (n == 0) return;

    size_t prevIdx = n - 1;
    bool prevInside = IsInside(poly[prevIdx], plane);

    for (size_t i = 0; i < n; ++i) {
        bool currInside = IsInside(poly[i], plane);

        if (currInside != prevInside) {
            if (prevInside) {
                float alpha = ComputeAlpha(poly[prevIdx], poly[i], plane);
                output.push_back(poly[prevIdx] + (poly[i] - poly[prevIdx]) * alpha);
            }
            else {
                float alpha = ComputeAlpha(poly[i], poly[prevIdx], plane);
                output.push_back(poly[i] + (poly[prevIdx] - poly[i]) * alpha);
            }
        }
        if (currInside)
            output.push_back(poly[i]);
        prevIdx = i;
        prevInside = currInside;
    }
}

template<typename Vertex>
static bool clipLine(Vertex &a, Vertex &b) {
    for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right,
                            ClipPlane::Bottom, ClipPlane::Top,
                            ClipPlane::Near, ClipPlane::Far}) {
        bool aInside = IsInside(a, plane);
        bool bInside = IsInside(b, plane);

        if (aInside && bInside) {
        continue;
        }

        if (!aInside && !bInside) {
        return false;
        }

        if (aInside) {
        float alpha = ComputeAlpha(a, b, plane);
        b.clip = a.clip + (b.clip - a.clip) * alpha;
        } else {
        float alpha = ComputeAlpha(b, a, plane);
        a.clip = b.clip + (a.clip - b.clip) * alpha;
        }
    }
    return true;
}

template<typename Vertex>
bool IsInside(const Vertex& v, ClipPlane plane) {
    const auto& p = v.clip;
    switch (plane) {
    case ClipPlane::Left:   return p.x >= -p.w;
    case ClipPlane::Right:  return p.x <= p.w;
    case ClipPlane::Bottom: return p.y >= -p.w;
    case ClipPlane::Top:    return p.y <= p.w;
    case ClipPlane::Near:   return p.z >= -p.w;
    case ClipPlane::Far:    return p.z <= p.w;
    }
    return false;
}

template<typename Vertex>
float ComputeAlpha(const Vertex& a, const Vertex& b, ClipPlane plane) {
    const auto& pa = a.clip;
    const auto& pb = b.clip;
    float num, denom;

    switch (plane) {
    case ClipPlane::Left:
        num = pa.x + pa.w; denom = (pa.x + pa.w) - (pb.x + pb.w); break;
    case ClipPlane::Right:
        num = pa.x - pa.w; denom = (pa.x - pa.w) - (pb.x - pb.w); break;
    case ClipPlane::Bottom:
        num = pa.y + pa.w; denom = (pa.y + pa.w) - (pb.y + pb.w); break;
    case ClipPlane::Top:
        num = pa.y - pa.w; denom = (pa.y - pa.w) - (pb.y - pb.w); break;
    case ClipPlane::Near:
        num = pa.z + pa.w; denom = (pa.z + pa.w) - (pb.z + pb.w); break;
    case ClipPlane::Far:
        num = pa.z - pa.w; denom = (pa.z - pa.w) - (pb.z - pb.w); break;
    }

    return denom != 0.0f ? num / denom : 0.0f;
}