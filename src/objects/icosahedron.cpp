#include <iostream>
#include <math.h>
#include <cstdint>
#include "icosahedron.hpp"
#include "../material.hpp"

void Icosahedron::loadVertices() {
    const float half = 50.f;
    const float axisDist = half * std::sqrt(3.f);  
    const float phi = (1.f + std::sqrt(5.f)) * 0.5f;

    // Unscaled canonical icosahedron vertices (12)
    std::vector<VertexData> v = {
        { -1.f,  phi,  0.f },  // 0
        {  1.f,  phi,  0.f },  // 1
        { -1.f, -phi,  0.f },  // 2
        {  1.f, -phi,  0.f },  // 3
        {  0.f, -1.f,  phi },  // 4
        {  0.f,  1.f,  phi },  // 5
        {  0.f, -1.f, -phi },  // 6
        {  0.f,  1.f, -phi },  // 7
        {  phi,  0.f, -1.f },  // 8
        {  phi,  0.f,  1.f },  // 9
        { -phi,  0.f, -1.f },  // 10
        { -phi,  0.f,  1.f }   // 11
    };

    for (auto& vt : v) {
        slib::vec3 p = vt.vertex;
        const float len = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        if (len > 0.f) {
            p = p * (axisDist / len);
        }
        vt.vertex = p;
        vt.texCoord = { (p.x / axisDist + 1.f) * 0.5f, (p.y / axisDist + 1.f) * 0.5f };
    }

    Icosahedron::vertexData = v;
    Icosahedron::numVertices = v.size();
}

void Icosahedron::loadFaces() {
    std::vector<FaceData> faces;

    MaterialProperties props = getMaterialProperties(MaterialType::Light);
    std::string mtlPath = "checker-map_tho.png";

    Material mat{};
    mat.Ka = { props.k_a * 0xff, props.k_a * 0xff, props.k_a * 0xff };
    mat.Kd = { props.k_d * 0xff, props.k_d * 0xff, props.k_d * 0xff };
    mat.Ks = { props.k_s * 0xff, props.k_s * 0xff, props.k_s * 0xff };
    mat.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    mat.map_Kd.setFilter(TextureFilter::NEIGHBOUR);
    mat.Ns = props.shininess;
    materials.insert({ "white", mat });

    // 20 triangular faces of the icosahedron (indices match the vertex order above)
    const uint16_t F[20][3] = {
        {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
        {1,5,9},  {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
        {3,9,4},  {3,4,2},  {3,2,6},  {3,6,8},  {3,8,9},
        {4,9,5},  {2,4,11}, {6,2,10}, {8,6,7},  {9,8,1}
    };

    faces.reserve(20);
    for (int i = 0; i < 20; ++i) {
        FaceData fd;
        fd.face.vertexIndices.push_back(F[i][0]);
        fd.face.vertexIndices.push_back(F[i][1]);
        fd.face.vertexIndices.push_back(F[i][2]);

        fd.face.materialKey = "white";
        faces.push_back(fd);
    }

    Icosahedron::faceData = faces;
    Icosahedron::numFaces = faces.size();
}
