#include <iostream>
#include <math.h>
#include <cstdint>
#include "test.hpp"
#include "../material.hpp"


void Test::loadVertices() {
    const float half = 10.f;
    const float axisDist = half * std::sqrt(3.f);  // ≈ 56755

    std::vector<VertexData> vertices;
    vertices.push_back({   axisDist,  axisDist,  axisDist });
    vertices.push_back({  -axisDist,  axisDist,  axisDist });
    vertices.push_back({  -axisDist, -axisDist,  axisDist });
    vertices.push_back({   axisDist, -axisDist,  axisDist });

    vertices.push_back({   axisDist,  axisDist, -axisDist });
    vertices.push_back({  -axisDist,  axisDist, -axisDist });
    vertices.push_back({  -axisDist, -axisDist, -axisDist });
    vertices.push_back({   axisDist, -axisDist, -axisDist });
    this->vertexData = vertices;
    this->numVertices = vertices.size();
}

void Test::loadFaces() {

    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);

    
    std::vector<FaceData> faces;

    // Create and store the material
    Material material{};
    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x58, properties.k_a * 0xfc };
    material.Kd = { properties.k_d * 0x00, properties.k_d * 0x58, properties.k_d * 0xfc }; 
    material.Ks = { properties.k_s * 0x00, properties.k_s * 0x58, properties.k_s * 0xfc };
    material.Ns = properties.shininess;
    materials.insert({"blue", material});

    material.Ka = { properties.k_a * 0xff, properties.k_a * 0xff, properties.k_a * 0xff };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0xff, properties.k_d * 0xff };
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    materials.insert({"white", material});

    FaceData face1;

    face1.face.vertexIndices.push_back(0 + 4);
	face1.face.vertexIndices.push_back(1 + 4);
	face1.face.vertexIndices.push_back(2 + 4);
    face1.face.materialKey = "blue";
    faces.push_back(face1);

	FaceData face2;

	face2.face.vertexIndices.push_back(0 + 4);
	face2.face.vertexIndices.push_back(2 + 4);
	face2.face.vertexIndices.push_back(3 + 4);
    face2.face.materialKey = "white";
    faces.push_back(face2);

    FaceData face3;

	face3.face.vertexIndices.push_back(0);
	face3.face.vertexIndices.push_back(1);
	face3.face.vertexIndices.push_back(2);
    face3.face.materialKey = "blue";
    faces.push_back(face3);

    FaceData face4;

	face4.face.vertexIndices.push_back(0);
	face4.face.vertexIndices.push_back(2);
	face4.face.vertexIndices.push_back(3);
    face4.face.materialKey = "white";
    faces.push_back(face4);

    this->faceData = faces;
    this->numFaces = faces.size();

}