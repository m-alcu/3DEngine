#pragma once
#include <string>
#include <render3d/ecs/mesh_component.hpp>
#include <render3d/ecs/material_component.hpp>
#include <render3d/ecs/transform_component.hpp>


using namespace render3d;

namespace PrefabFactory {

    void buildCube(MeshComponent& mesh, MaterialComponent& material);
    void buildPlane(MeshComponent& mesh, MaterialComponent& material, float size);
    void buildTorus(MeshComponent& mesh, MaterialComponent& material,
                    int uSteps, int vSteps, float R, float r);
    void buildKnot(MeshComponent& mesh, MaterialComponent& material,
                   int lobes, int uSteps, int vSteps, float scale, float r);
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
