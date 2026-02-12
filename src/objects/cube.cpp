#include <iostream>
#include <math.h>
#include <cstdint>
#include "cube.hpp"
#include "../material.hpp"


void Cube::loadVertices() {
    const float half = 10.f;

    std::vector<VertexData> vertices;
    VertexData v;

    // Front face (z = +half)
    v.vertex = { -half, -half, +half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { +half, -half, +half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
    v.vertex = { +half, +half, +half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { -half, +half, +half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

    // Back face (z = -half)
    v.vertex = { +half, -half, -half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { -half, -half, -half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
    v.vertex = { -half, +half, -half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { +half, +half, -half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

    // Left face (x = -half)
    v.vertex = { -half, -half, -half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { -half, -half, +half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
    v.vertex = { -half, +half, +half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { -half, +half, -half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

    // Right face (x = +half)
    v.vertex = { +half, -half, +half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { +half, -half, -half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
    v.vertex = { +half, +half, -half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { +half, +half, +half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

    // Top face (y = +half)
    v.vertex = { -half, +half, +half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { +half, +half, +half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
    v.vertex = { +half, +half, -half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { -half, +half, -half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

    // Bottom face (y = -half)
    v.vertex = { -half, -half, -half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
    v.vertex = { +half, -half, -half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
    v.vertex = { +half, -half, +half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
    v.vertex = { -half, -half, +half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

    this->mesh->vertexData = vertices;
    this->mesh->numVertices = vertices.size();
}

void Cube::loadFaces() {
    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);

    std::string materialKey = "floorTexture";
    std::string mtlPath = "checker-map_tho.png";

    // Create and store the material
    Material material{};

    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0xff, properties.k_d * 0xff };
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    material.map_Kd = LoadTextureFromImg(std::string(RES_PATH + mtlPath).c_str());
    mesh->materials.insert({materialKey, material});

    // Each face has 2 triangles, so for each face we generate 6 indices
    for (int baseIndex = 0; baseIndex < 4*6; baseIndex += 4) {

        FaceData face;
		face.face.vertexIndices = { baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 3 };
		face.face.materialKey = materialKey;
                
        this->mesh->faceData.push_back(face);
    }

    this->mesh->numFaces = this->mesh->faceData.size();
}
