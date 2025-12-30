#pragma once
#include <vector>
#include "polygon.hpp"
#include "scene.hpp" // If needed for material, etc.
#include "projection.hpp"

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
Polygon<Vertex> ClipCullPolygonSutherlandHodgman(const Polygon<Vertex>& t, const Scene& scene) {
    std::vector<Vertex> polygon = t.points;

    for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right, ClipPlane::Bottom,
        ClipPlane::Top, ClipPlane::Near, ClipPlane::Far}) {
        polygon = ClipAgainstPlane(polygon, plane, scene);
        if (polygon.empty()) return Polygon<Vertex>(polygon, t.face, t.rotatedFaceNormal, t.material); // Completely outside
    }

    return Polygon<Vertex>(polygon, t.face, t.rotatedFaceNormal, t.material);
}

template<typename Vertex>
std::vector<Vertex> ClipAgainstPlane(const std::vector<Vertex>& poly, ClipPlane plane, const Scene& scene) {
    std::vector<Vertex> output;
    if (poly.empty()) return output;

    Vertex prev = poly.back();
    bool prevInside = IsInside(prev, plane);

    for (const auto& curr : poly) {
        bool currInside = IsInside(curr, plane);

        if (currInside != prevInside) {
            // from inside to outside, we need to clip the edge always this way
            if (prevInside) {
                float alpha = ComputeAlpha(prev, curr, plane);
				Vertex v = InterpolateVertex(prev, curr, alpha, scene);
                output.push_back(v);
            }
            else {
                float alpha = ComputeAlpha(curr, prev, plane);
				Vertex v = InterpolateVertex(curr, prev, alpha, scene);
                output.push_back(v);
            }
        }
        if (currInside)
            output.push_back(curr);
        prev = curr;
        prevInside = currInside;
    }

    return output;
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

template<typename Vertex>
Vertex InterpolateVertex(const Vertex& a, const Vertex& b, float alpha, const Scene& scene) {
	Vertex result;
    Projection<Vertex> projection;
	result = a + (b - a) * alpha;

    if constexpr (has_tex<Vertex>) {
		result.tex.x = (a.tex.x * a.ndc.w) + ((b.tex.x * b.ndc.w) - (a.tex.x * a.ndc.w)) * alpha;
		result.tex.y = (a.tex.y * a.ndc.w) + ((b.tex.y * b.ndc.w) - (a.tex.y * a.ndc.w)) * alpha;
        result.tex.w = 1.0f;
    }

    projection.view(scene, result);

	// Interpolate other attributes as needed (e.g., color, texture coordinates)
	return result;
}