#include "plane.hpp"
#include "../material.hpp"
#include "../ecs/MaterialSystem.hpp"

void Plane::loadVertices() {
    const float half = size;

    std::vector<VertexData> vertices;
    VertexData v;

    // Single face plane (facing to screen, z = 0)
    v.vertex = { -half, -half, 0}; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { +half, -half, 0}; v.texCoord = { 1, 0 }; vertices.push_back(v);
    v.vertex = { +half, +half, 0}; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { -half, +half, 0} ; v.texCoord = { 0, 1 }; vertices.push_back(v);

    this->mesh->vertexData = vertices;
    this->mesh->numVertices = vertices.size();
}

void Plane::loadFaces() {
    MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Plastic);

    std::string materialKey = "planeMaterial";

    Material material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0x40, 0x40, 0x40},
        slib::vec3{0xaa, 0xaa, 0xaa},
        slib::vec3{0xff, 0xff, 0xff}
    );
    materialComponent->materials.insert({materialKey, material});

    FaceData face;
    face.face.vertexIndices = { 0, 1, 2, 3 };
    face.face.materialKey = materialKey;
    this->mesh->faceData.push_back(face);

    this->mesh->numFaces = this->mesh->faceData.size();
}
