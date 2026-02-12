#include "plane.hpp"
#include "../material.hpp"

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
    MaterialProperties properties = getMaterialProperties(MaterialType::Plastic);

    std::string materialKey = "planeMaterial";

    Material material{};
    material.Ka = { properties.k_a * 0x40, properties.k_a * 0x40, properties.k_a * 0x40 };
    material.Kd = { properties.k_d * 0xaa, properties.k_d * 0xaa, properties.k_d * 0xaa };
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    mesh->materials.insert({materialKey, material});

    FaceData face;
    face.face.vertexIndices = { 0, 1, 2, 3 };
    face.face.materialKey = materialKey;
    this->mesh->faceData.push_back(face);

    this->mesh->numFaces = this->mesh->faceData.size();
}
