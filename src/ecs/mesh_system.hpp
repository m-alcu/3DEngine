#pragma once
#include <vector>
#include "mesh_component.hpp"
#include "component_store.hpp"
#include "../smath.hpp"

namespace MeshSystem {

    inline void updateFaceNormals(MeshComponent& mesh) {
        const int nFaces = static_cast<int>(mesh.faceData.size());
        if (nFaces == 0) return;

        for (int i = 0; i < nFaces; i++) {
            const Face &face = mesh.faceData[i].face;
            const size_t n = face.vertexIndices.size();

            slib::vec3 normal = {0.0f, 0.0f, 0.0f};
            if (n == 0) {
                mesh.faceData[i].faceNormal = normal;
                continue;
            }

            for (size_t j = 0; j < n; ++j) {
                const slib::vec3& curr = mesh.vertexData[face.vertexIndices[j]].vertex;
                const slib::vec3& next = mesh.vertexData[face.vertexIndices[(j + 1) % n]].vertex;

                normal.x += (curr.y - next.y) * (curr.z + next.z);
                normal.y += (curr.z - next.z) * (curr.x + next.x);
                normal.z += (curr.x - next.x) * (curr.y + next.y);
            }
            mesh.faceData[i].faceNormal = smath::normalize(normal);
        }
    }

    inline void updateVertexNormals(MeshComponent& mesh) {
        const int nVerts = static_cast<int>(mesh.vertexData.size());
        const int nFaces = static_cast<int>(mesh.faceData.size());
        if (nVerts == 0) return;

        std::vector<slib::vec3> acc(nVerts, {0.0f, 0.0f, 0.0f});
        for (int j = 0; j < nFaces; j++) {
            for (int vi : mesh.faceData[j].face.vertexIndices) {
                acc[vi] += mesh.faceData[j].faceNormal;
            }
        }
        for (int i = 0; i < nVerts; i++) {
            mesh.vertexData[i].normal = smath::normalize(acc[i]);
        }
    }

    inline void updateRadius(MeshComponent& mesh) {
        const int nVerts = static_cast<int>(mesh.vertexData.size());
        mesh.radius = 0.0f;
        for (int i = 0; i < nVerts; i++) {
            float d = smath::distance(mesh.vertexData[i].vertex);
            if (d > mesh.radius) mesh.radius = d;
        }
        mesh.boundsDirty = false;
    }

    inline void markBoundsDirty(MeshComponent& mesh) {
        mesh.boundsDirty = true;
    }

    inline void updateBoundsIfDirty(MeshComponent& mesh) {
        if (mesh.boundsDirty) {
            updateRadius(mesh);
        }
    }

    inline void updateAllFaceNormals(ComponentStore<MeshComponent>& store) {
        for (auto& [entity, mesh] : store) {
            updateFaceNormals(mesh);
        }
    }

    inline void updateAllVertexNormals(ComponentStore<MeshComponent>& store) {
        for (auto& [entity, mesh] : store) {
            updateVertexNormals(mesh);
        }
    }

    inline void updateAllBounds(ComponentStore<MeshComponent>& store) {
        for (auto& [entity, mesh] : store) {
            updateRadius(mesh);
        }
    }

    inline void updateAllBoundsIfDirty(ComponentStore<MeshComponent>& store) {
        for (auto& [entity, mesh] : store) {
            updateBoundsIfDirty(mesh);
        }
    }

} // namespace MeshSystem
