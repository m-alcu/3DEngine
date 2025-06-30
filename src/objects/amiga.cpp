#include <iostream>
#include <math.h>
#include <cstdint>
#include "amiga.hpp"
#include "../smath.hpp"
#include "../constants.hpp"

void Amiga::loadVertices() {
}

void Amiga::loadFaces() {
}

void Amiga::setup(int lat, int lon) {
    loadVertices(lat, lon);
    loadFaces(lat, lon);
    calculateNormals();
    calculateVertexNormals();
}

bool Amiga::isRedTile(float u, float v, int rows, int cols) {
    int x = (int)(u * cols);
    int y = (int)(v * rows);
    return (x + y) % 2 == 0;    
}

void Amiga::loadVertices(int lat, int lon) {

    std::vector<VertexData> vertices;
    vertices.resize((lat + 1) * lon);
    Amiga::vertexData = vertices;


    for (int i = 0; i <= lat; i++) {
        float theta = i * PI / lat;
        for (int j = 0; j < lon; j++) {
            float phi = j * 2 * PI / lon;
            float x = sinf(theta) * cosf(phi);
            float y = cosf(theta);
            float z = sinf(theta) * sinf(phi);
            Amiga::vertexData[i * lon + j].vertex = { x, y, z };

            // Ensure texture wraps by duplicating the last column with u = 1.0 when j == lon
            float u = phi / (2 * PI);
            float v = theta / PI;
            Amiga::vertexData[i * lon + j].texCoord = { u, v };

        }
    }    

    Amiga::numVertices = vertices.size();
}

void Amiga::calculateVertexNormals() {

    for (int i = 0; i < numVertices; i++) {
        Amiga::vertexData[i].normal = Amiga::vertexData[i].vertex; // Initialize normals to zero
    }
}

void Amiga::loadFaces(int lat, int lon) {
    // Each quad gives rise to 2 triangles.
    // Total faces = uSteps * vSteps * 2

    std::vector<FaceData> faces;

    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);

    std::string mtlPath = "checker-map_tho.png";

    slib::material material{};
    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0x00, properties.k_d * 0x00 }; 
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    material.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    material.map_Kd.textureFilter = slib::TextureFilter::NEIGHBOUR;
    materials.insert({"red", material});

    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0xff, properties.k_d * 0xff };
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    material.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    material.map_Kd.textureFilter = slib::TextureFilter::NEIGHBOUR;
    materials.insert({"white", material});  

for (int i = 0; i < lat; i++) {
    for (int j = 0; j < lon; j++) {
        int jNext = (j + 1) % lon; // wraps last column to the first

        int row1 = i * lon;
        int row2 = (i + 1) * lon;

        int v1 = row1 + j;
        int v2 = row2 + j;
        int v3 = row1 + jNext;
        int v4 = row2 + jNext;

        // Optional: base color on (u,v)
        std::string color = isRedTile(j / (float)lon, i / (float)lat, lat, lon) ? "red" : "white";

        // Triangle 1
        FaceData face1;
        face1.face.vertex1 = v3;
        face1.face.vertex2 = v2;
        face1.face.vertex3 = v1;
        face1.face.materialKey = color;
        faces.push_back(face1);

        // Triangle 2
        FaceData face2;
        face2.face.vertex1 = v4;
        face2.face.vertex2 = v2;
        face2.face.vertex3 = v3;
        face2.face.materialKey = color;
        faces.push_back(face2);
    }
}

    

    Amiga::faceData = faces;
    Amiga::numFaces = faces.size();
}