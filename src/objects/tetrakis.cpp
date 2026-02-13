#include <iostream>
#include <math.h>
#include <cstdint>
#include "tetrakis.hpp"
#include "../material.hpp"
#include "../ecs/MaterialSystem.hpp"

void Tetrakis::loadVertices() {
    const float half = 50.f;
    const float axisDist = half * std::sqrt(3.f);  

    std::vector<VertexData> vertices;

    int index = 0;
    // Generate the 8 cube vertices with explicit sign choices
    for (int xSign : {1, -1}) {
        for (int ySign : {1, -1}) {
            for (int zSign : {1, -1}) {
                vertices.push_back({ half * xSign, half * ySign, half * zSign });
            }
        }
    }

    // Generate the 6 axis-aligned vertices

    vertices.push_back({ axisDist, 0, 0 });
    vertices.push_back({ 0, axisDist, 0 });
    vertices.push_back({ 0, 0, axisDist });
    vertices.push_back({ -axisDist, 0, 0 });
    vertices.push_back({ 0, -axisDist, 0 });
    vertices.push_back({ 0, 0, -axisDist });

    mesh->vertexData = vertices;

    for (auto& vertex : mesh->vertexData) {
        vertex.texCoord = { (vertex.vertex.x/axisDist + 1)/2, (vertex.vertex.y/axisDist + 1)/2 };
    }

    mesh->numVertices = vertices.size();
}

void Tetrakis::loadFaces() {

    std::vector<FaceData> faces;

    MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);

    std::string mtlPath = "checker-map_tho.png";
    Material material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0x00, 0x58, 0xfc},
        slib::vec3{0x00, 0x58, 0xfc},
        slib::vec3{0x00, 0x58, 0xfc},
        std::string(RES_PATH + mtlPath),
        TextureFilter::NEIGHBOUR
    );
    materialComponent->materials.insert({"blue", material});

    material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0xff, 0xff, 0xff},
        slib::vec3{0xff, 0xff, 0xff},
        slib::vec3{0xff, 0xff, 0xff},
        std::string(RES_PATH + mtlPath),
        TextureFilter::NEIGHBOUR
    );
    materialComponent->materials.insert({"white", material});    

    // Define the quadrilaterals (outer vertices) and centers for each face group.
    const uint16_t quads[6][4] = {
        {2, 0, 1, 3},  // group 0, center 8
        {4, 5, 1, 0},  // group 1, center 9
        {2, 6, 4, 0},  // group 2, center 10
        {4, 6, 7, 5},  // group 3, center 11
        {7, 6, 2, 3},  // group 4, center 12
        {1, 5, 7, 3}   // group 5, center 13
    };
    const uint16_t centers[6] = {8, 9, 10, 11, 12, 13};

    // There are 6 groups, each generating 4 faces (triangles) = 24 total faces.
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 4; j++) {
            int faceIndex = i * 4 + j;
            // Directly assign values to the face.
            uint32_t color = ((j % 2 == 0) ? 0xff0058fc : 0Xffffffff);

            FaceData face;

            face.face.vertexIndices.push_back(quads[i][(j + 1) % 4]);
            face.face.vertexIndices.push_back(quads[i][j]);
            face.face.vertexIndices.push_back(centers[i]);

            if (j % 2 == 0) {
                face.face.materialKey = "blue";
            } else {
                face.face.materialKey = "white";
            }

            faces.push_back(face);
        }
    }

    mesh->faceData = faces;
    mesh->numFaces = faces.size();
}
