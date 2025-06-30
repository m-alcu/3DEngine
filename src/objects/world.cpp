#include <iostream>
#include <math.h>
#include <cstdint>
#include "world.hpp"
#include "../smath.hpp"
#include "../constants.hpp"

void World::loadVertices() {
}

void World::loadFaces() {
}

void World::setup(int lat, int lon) {
    loadVertices(lat, lon);
    loadFaces(lat, lon);
    calculateNormals();
    calculateVertexNormals();
}

bool World::isRedTile(float u, float v, int rows, int cols) {
    int x = (int)(u * cols);
    int y = (int)(v * rows);
    return (x + y) % 2 == 0;    
}

void World::loadVertices(int lat, int lon) {

    std::vector<VertexData> vertices;
    vertices.resize((lat + 1) * (lon + 1));
    World::vertexData = vertices;


    for (int i = 0; i <= lat; i++) {
        float theta = i * PI / lat;
        for (int j = 0; j <= lon; j++) {
            float phi = j * 2 * PI / lon;
            if (j == lon) {
                World::vertexData[i * (lon + 1) + j].vertex = World::vertexData[i * (lon + 1) + j - lon].vertex;
            } else {
                float x = sinf(theta) * cosf(phi);
                float y = cosf(theta);
                float z = sinf(theta) * sinf(phi);
                World::vertexData[i * (lon + 1) + j].vertex = { x, y, z };
            }
            // Ensure texture wraps by duplicating the last column with u = 1.0 when j == lon
            float u = (j == lon) ? 1.0f : phi / (2 * PI);
            float v = (i == lat) ? 1.0f : theta / PI;
            World::vertexData[i * (lon + 1) + j].texCoord = { u, v };

        }
    }    

    World::numVertices = vertices.size();
}

void World::calculateVertexNormals() {

    for (int i = 0; i < numVertices; i++) {
        World::vertexData[i].normal = World::vertexData[i].vertex; // Initialize normals to zero
    }
}

void World::loadFaces(int lat, int lon) {
    // Each quad gives rise to 2 triangles.
    // Total faces = uSteps * vSteps * 2

    std::vector<FaceData> faces;

    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);

    std::string mtlPath = "earth_texture.png";

    slib::material material{};
    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0x00, properties.k_d * 0x00 }; 
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    material.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    material.map_Kd.textureFilter = slib::TextureFilter::BILINEAR;
    materials.insert({"red", material});

    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0xff, properties.k_d * 0xff };
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.Ns = properties.shininess;
    material.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    material.map_Kd.textureFilter = slib::TextureFilter::BILINEAR;
    materials.insert({"white", material});  

for (int i = 0; i < lat; i++) {
    for (int j = 0; j < lon; j++) {

        int row1 = i * (lon + 1);
        int row2 = (i + 1) * (lon + 1);

        int v1 = row1 + j;
        int v2 = row2 + j;
        int v3 = row1 + j + 1;
        int v4 = row2 + j + 1;

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

    

    World::faceData = faces;
    World::numFaces = faces.size();
}