#pragma once
#include "mesh_component.hpp"
#include "component_store.hpp"
#include "../smath.hpp"

namespace MeshSystem {

    inline void updateFaceNormals(MeshComponent& mesh) {
        if (mesh.numFaces == 0) return;

        for (int i = 0; i < mesh.numFaces; i++) {
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
        if (mesh.numVertices == 0) return;

        for (int i = 0; i < mesh.numVertices; i++) {
            slib::vec3 vertexNormal = {0.0f, 0.0f, 0.0f};
            for(int j = 0; j < mesh.numFaces; j++) {
                for (int vi : mesh.faceData[j].face.vertexIndices) {
                    if (vi == i) {
                        vertexNormal += mesh.faceData[j].faceNormal;
                    }
                }
            }
            mesh.vertexData[i].normal = smath::normalize(vertexNormal);
        }
    }

    inline void updateRadius(MeshComponent& mesh) {
        mesh.radius = 0.0f;
        for (int i = 0; i < mesh.numVertices; i++) {
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
