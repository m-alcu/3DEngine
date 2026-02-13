#include <iostream>
#include <math.h>
#include <cstdint>
#include "test.hpp"
#include "../material.hpp"
#include "../ecs/MaterialSystem.hpp"


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
    this->mesh->vertexData = vertices;
    this->mesh->numVertices = vertices.size();
}

void Test::loadFaces() {

    MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);

    
    std::vector<FaceData> faces;

    Material material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0x00, 0x58, 0xfc},
        slib::vec3{0x00, 0x58, 0xfc},
        slib::vec3{0x00, 0x58, 0xfc}
    );
    materialComponent->materials.insert({"blue", material});

    material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0xff, 0xff, 0xff},
        slib::vec3{0xff, 0xff, 0xff},
        slib::vec3{0xff, 0xff, 0xff}
    );
    materialComponent->materials.insert({"white", material});

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

    this->mesh->faceData = faces;
    this->mesh->numFaces = faces.size();

}
