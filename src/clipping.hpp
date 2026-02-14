#pragma once
#include <vector>
#include "polygon.hpp"
#include "scene.hpp" // If needed for material, etc.

enum class ClipPlane {
    Left, Right, Bottom, Top, Near, Far
};

/*
Clipping is done using the Sutherland-Hodgman algorithm (1974) in the ndc space.
The Sutherland-Hodgman algorithm is a polygon clipping algorithm that clips a polygon against a convex clipping region.
The algorithm works by iterating through each edge of the polygon and checking if the vertices are inside or outside the clipping plane.
If a vertex is inside, it is added to the output polygon. If a vertex is outside, the algorithm checks if the previous vertex was inside. If it was, the edge between the two vertices is clipped and the intersection point is added to the output polygon.
The algorithm continues until all edges have been processed.
https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
*/


template<typename Vertex>
Polygon<Vertex> ClipCullPolygon(const Polygon<Vertex>& t) {
    // Early exit: check if all vertices are inside all clip planes
    // This avoids expensive clipping for polygons entirely within the view frustum
    bool allInside = true;
    for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right, ClipPlane::Bottom,
                            ClipPlane::Top, ClipPlane::Near, ClipPlane::Far}) {
        for (const auto& v : t.points) {
            if (!IsInside(v, plane)) {
                allInside = false;
                break;
            }
        }
        if (!allInside) break;
    }

    // If completely inside, return original polygon unchanged
    if (allInside) {
        return t;
    }

    // Otherwise, perform clipping with double-buffered vectors
    std::vector<Vertex> bufA = t.points;
    std::vector<Vertex> bufB;
    bufB.reserve(bufA.size() + 6);

    for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right, ClipPlane::Bottom,
        ClipPlane::Top, ClipPlane::Near, ClipPlane::Far}) {
        ClipAgainstPlane(bufA, bufB, plane);
        if (bufB.empty()) {
            if (t.material) {
                return Polygon<Vertex>(std::move(bufB), t.rotatedFaceNormal, *t.material);
            } else {
                return Polygon<Vertex>(std::move(bufB), t.rotatedFaceNormal);
            }
        }
        std::swap(bufA, bufB);
    }

    if (t.material) {
        return Polygon<Vertex>(std::move(bufA), t.rotatedFaceNormal, *t.material);
    } else {
        return Polygon<Vertex>(std::move(bufA), t.rotatedFaceNormal);
    }
}

template<typename Vertex>
void ClipAgainstPlane(const std::vector<Vertex>& poly, std::vector<Vertex>& output, ClipPlane plane) {
    output.clear();
    if (poly.empty()) return;

    Vertex prev = poly.back();
    bool prevInside = IsInside(prev, plane);

    for (const auto& curr : poly) {
        bool currInside = IsInside(curr, plane);

        if (currInside != prevInside) {
            // from inside to outside, we need to clip the edge always this way
            if (prevInside) {
                float alpha = ComputeAlpha(prev, curr, plane);
                output.push_back(prev + (curr - prev) * alpha);
            }
            else {
                float alpha = ComputeAlpha(curr, prev, plane);
                output.push_back(curr + (prev - curr) * alpha);
            }
        }
        if (currInside)
            output.push_back(curr);
        prev = curr;
        prevInside = currInside;
    }
}

template<typename Vertex>
static bool clipLineNdc(Vertex &a, Vertex &b) {
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
        b.ndc = a.ndc + (b.ndc - a.ndc) * alpha;
        } else {
        float alpha = ComputeAlpha(b, a, plane);
        a.ndc = b.ndc + (a.ndc - b.ndc) * alpha;
        }
    }
    return true;
}

template<typename Vertex>
bool IsInside(const Vertex& v, ClipPlane plane) {
    const auto& p = v.ndc;
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
    const auto& pa = a.ndc;
    const auto& pb = b.ndc;
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