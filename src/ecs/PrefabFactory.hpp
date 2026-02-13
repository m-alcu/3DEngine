#pragma once
#include <string>
#include "MeshComponent.hpp"
#include "MaterialComponent.hpp"
#include "TransformComponent.hpp"

namespace PrefabFactory {

    void buildCube(MeshComponent& mesh, MaterialComponent& material);
    void buildPlane(MeshComponent& mesh, MaterialComponent& material, float size);
    void buildTorus(MeshComponent& mesh, MaterialComponent& material,
                    int uSteps, int vSteps, float R, float r);
    void buildWorld(MeshComponent& mesh, MaterialComponent& material,
                    int lat, int lon);
    void buildAmiga(MeshComponent& mesh, MaterialComponent& material,
                    int lat, int lon);
    void buildTetrakis(MeshComponent& mesh, MaterialComponent& material);
    void buildIcosahedron(MeshComponent& mesh, MaterialComponent& material);
    void buildTest(MeshComponent& mesh, MaterialComponent& material);

    bool buildObj(const std::string& filename, MeshComponent& mesh,
                  MaterialComponent& material, TransformComponent& transform);
    void buildAsc(const std::string& filename, MeshComponent& mesh,
                  MaterialComponent& material);

} // namespace PrefabFactory
