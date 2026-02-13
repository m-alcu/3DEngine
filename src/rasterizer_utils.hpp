#pragma once
#include <vector>
#include "ecs/MeshComponent.hpp"

template<class V>
inline std::vector<V> collectPolyVerts(const std::vector<V>& projectedPoints,
                                       const FaceData& faceDataEntry) {
    std::vector<V> polyVerts;
    polyVerts.reserve(faceDataEntry.face.vertexIndices.size());
    for (int j : faceDataEntry.face.vertexIndices) {
        polyVerts.push_back(projectedPoints[j]);
    }
    return polyVerts;
}
