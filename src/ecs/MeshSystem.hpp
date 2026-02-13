#pragma once
#include "MeshComponent.hpp"
#include "ComponentStore.hpp"
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

    inline void updateMinMaxCoords(MeshComponent& mesh) {
        if (mesh.numVertices == 0) {
            mesh.minCoord = {0.0f, 0.0f, 0.0f};
            mesh.maxCoord = {0.0f, 0.0f, 0.0f};
            mesh.boundsDirty = false;
            return;
        }

        mesh.minCoord = mesh.vertexData[0].vertex;
        mesh.maxCoord = mesh.vertexData[0].vertex;

        for (int i = 1; i < mesh.numVertices; i++) {
            const slib::vec3& v = mesh.vertexData[i].vertex;

            if (v.x < mesh.minCoord.x) mesh.minCoord.x = v.x;
            if (v.y < mesh.minCoord.y) mesh.minCoord.y = v.y;
            if (v.z < mesh.minCoord.z) mesh.minCoord.z = v.z;

            if (v.x > mesh.maxCoord.x) mesh.maxCoord.x = v.x;
            if (v.y > mesh.maxCoord.y) mesh.maxCoord.y = v.y;
            if (v.z > mesh.maxCoord.z) mesh.maxCoord.z = v.z;
        }
        mesh.boundsDirty = false;
    }

    inline float getBoundingRadius(const MeshComponent& mesh) {
        if (mesh.numVertices == 0) return 0.0f;
        slib::vec3 center{(mesh.minCoord.x + mesh.maxCoord.x) * 0.5f,
                          (mesh.minCoord.y + mesh.maxCoord.y) * 0.5f,
                          (mesh.minCoord.z + mesh.maxCoord.z) * 0.5f};
        slib::vec3 halfDiag{mesh.maxCoord.x - center.x,
                            mesh.maxCoord.y - center.y,
                            mesh.maxCoord.z - center.z};
        return smath::distance(halfDiag);
    }

    inline void markBoundsDirty(MeshComponent& mesh) {
        mesh.boundsDirty = true;
    }

    inline void updateBoundsIfDirty(MeshComponent& mesh) {
        if (mesh.boundsDirty) {
            updateMinMaxCoords(mesh);
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
            updateMinMaxCoords(mesh);
        }
    }

    inline void updateAllBoundsIfDirty(ComponentStore<MeshComponent>& store) {
        for (auto& [entity, mesh] : store) {
            updateBoundsIfDirty(mesh);
        }
    }

} // namespace MeshSystem
