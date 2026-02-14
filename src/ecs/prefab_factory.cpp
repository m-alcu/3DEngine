#include "prefab_factory.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <tuple>
#include <vector>
#include "../constants.hpp"
#include "../material.hpp"
#include "../smath.hpp"
#include "material_system.hpp"
#include "mesh_system.hpp"
#include "transform_system.hpp"
#include <rapidobj/rapidobj.hpp>

namespace {

    bool isRedTile(float u, float v, int rows, int cols) {
        int x = static_cast<int>(u * cols);
        int y = static_cast<int>(v * rows);
        return (x + y) % 2 == 0;
    }

    void setVertexNormalsToPosition(MeshComponent& mesh) {
        for (int i = 0; i < mesh.numVertices; i++) {
            mesh.vertexData[i].normal = mesh.vertexData[i].vertex;
        }
    }

}

namespace PrefabFactory {

    void buildCube(MeshComponent& mesh, MaterialComponent& material) {
        const float half = 10.f;

        std::vector<VertexData> vertices;
        VertexData v;

        v.vertex = { -half, -half, +half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { +half, -half, +half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
        v.vertex = { +half, +half, +half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { -half, +half, +half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

        v.vertex = { +half, -half, -half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { -half, -half, -half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
        v.vertex = { -half, +half, -half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { +half, +half, -half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

        v.vertex = { -half, -half, -half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { -half, -half, +half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
        v.vertex = { -half, +half, +half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { -half, +half, -half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

        v.vertex = { +half, -half, +half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { +half, -half, -half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
        v.vertex = { +half, +half, -half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { +half, +half, +half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

        v.vertex = { -half, +half, +half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { +half, +half, +half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
        v.vertex = { +half, +half, -half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { -half, +half, -half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

        v.vertex = { -half, -half, -half }; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { +half, -half, -half }; v.texCoord = { 0, 1 }; vertices.push_back(v);
        v.vertex = { +half, -half, +half }; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { -half, -half, +half }; v.texCoord = { 1, 0 }; vertices.push_back(v);

        mesh.vertexData = vertices;
        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string materialKey = "floorTexture";
        std::string mtlPath = "checker-map_tho.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath)
        );
        material.materials.insert({materialKey, mat});

        for (int baseIndex = 0; baseIndex < 4 * 6; baseIndex += 4) {
            FaceData face;
            face.face.vertexIndices = { baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 3 };
            face.face.materialKey = materialKey;
            mesh.faceData.push_back(face);
        }

        mesh.numFaces = static_cast<int>(mesh.faceData.size());
        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildPlane(MeshComponent& mesh, MaterialComponent& material, float size) {
        const float half = size;

        std::vector<VertexData> vertices;
        VertexData v;

        v.vertex = { -half, -half, 0}; v.texCoord = { 0, 0 }; vertices.push_back(v);
        v.vertex = { +half, -half, 0}; v.texCoord = { 1, 0 }; vertices.push_back(v);
        v.vertex = { +half, +half, 0}; v.texCoord = { 1, 1 }; vertices.push_back(v);
        v.vertex = { -half, +half, 0} ; v.texCoord = { 0, 1 }; vertices.push_back(v);

        mesh.vertexData = vertices;
        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Plastic);
        std::string materialKey = "planeMaterial";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x40, 0x40, 0x40},
            slib::vec3{0xaa, 0xaa, 0xaa},
            slib::vec3{0xff, 0xff, 0xff}
        );
        material.materials.insert({materialKey, mat});

        FaceData face;
        face.face.vertexIndices = { 0, 1, 2, 3 };
        face.face.materialKey = materialKey;
        mesh.faceData.push_back(face);
        mesh.numFaces = static_cast<int>(mesh.faceData.size());
        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildTorus(MeshComponent& mesh, MaterialComponent& material,
                    int uSteps, int vSteps, float R, float r) {
        std::vector<VertexData> vertices;
        vertices.resize(uSteps * vSteps);
        mesh.vertexData = vertices;

        for (int i = 0; i < uSteps; i++) {
            float u = i * 2 * PI / uSteps;
            float cosU = std::cos(u);
            float sinU = std::sin(u);
            for (int j = 0; j < vSteps; j++) {
                float v = j * 2 * PI / vSteps;
                float cosV = std::cos(v);
                float sinV = std::sin(v);
                float x = (R + r * cosV) * cosU;
                float y = (R + r * cosV) * sinU;
                float z = r * sinV;
                mesh.vertexData[i * vSteps + j].vertex = { x, y, z };
                mesh.vertexData[i * vSteps + j].texCoord = { (x / (R + r) + 1) / 2, (y / (R + r) + 1) / 2 };
            }
        }

        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string mtlPath = "checker-map_tho.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0x00, 0x58, 0xfc},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"blue", mat});

        mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"white", mat});

        std::vector<FaceData> faces;
        for (int i = 0; i < uSteps; i++) {
            int nextI = (i + 1) % uSteps;
            for (int j = 0; j < vSteps; j++) {
                int nextJ = (j + 1) % vSteps;
                int idx0 = i * vSteps + j;
                int idx1 = nextI * vSteps + j;
                int idx2 = nextI * vSteps + nextJ;
                int idx3 = i * vSteps + nextJ;

                FaceData face1;
                face1.face.vertexIndices.push_back(idx0);
                face1.face.vertexIndices.push_back(idx1);
                face1.face.vertexIndices.push_back(idx2);
                face1.face.materialKey = "blue";
                faces.push_back(face1);

                FaceData face2;
                face2.face.vertexIndices.push_back(idx0);
                face2.face.vertexIndices.push_back(idx2);
                face2.face.vertexIndices.push_back(idx3);
                face2.face.materialKey = "white";
                faces.push_back(face2);
            }
        }

        mesh.faceData = faces;
        mesh.numFaces = static_cast<int>(faces.size());
        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildWorld(MeshComponent& mesh, MaterialComponent& material,
                    int lat, int lon) {
        std::vector<VertexData> vertices;
        vertices.resize((lat + 1) * (lon + 1));
        mesh.vertexData = vertices;

        for (int i = 0; i <= lat; i++) {
            float theta = i * PI / lat;
            for (int j = 0; j <= lon; j++) {
                float phi = j * 2 * PI / lon;
                if (j == lon) {
                    mesh.vertexData[i * (lon + 1) + j].vertex =
                        mesh.vertexData[i * (lon + 1) + j - lon].vertex;
                } else {
                    float x = std::sin(theta) * std::cos(phi);
                    float y = std::cos(theta);
                    float z = std::sin(theta) * std::sin(phi);
                    mesh.vertexData[i * (lon + 1) + j].vertex = { x, y, z };
                }
                float u = (j == lon) ? 1.0f : phi / (2 * PI);
                float v = (i == lat) ? 1.0f : theta / PI;
                mesh.vertexData[i * (lon + 1) + j].texCoord = { u, v };
            }
        }

        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string mtlPath = "earth_texture.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::BILINEAR_INT
        );
        material.materials.insert({"red", mat});

        mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::BILINEAR_INT
        );
        material.materials.insert({"white", mat});

        std::vector<FaceData> faces;
        for (int i = 0; i < lat; i++) {
            for (int j = 0; j < lon; j++) {
                int row1 = i * (lon + 1);
                int row2 = (i + 1) * (lon + 1);

                int v1 = row1 + j;
                int v2 = row2 + j;
                int v3 = row1 + j + 1;
                int v4 = row2 + j + 1;

                std::string color = isRedTile(j / (float)lon, i / (float)lat, lat, lon)
                    ? "red" : "white";

                FaceData face;
                face.face.vertexIndices.push_back(v4);
                face.face.vertexIndices.push_back(v2);
                face.face.vertexIndices.push_back(v1);
                face.face.vertexIndices.push_back(v3);
                face.face.materialKey = color;
                faces.push_back(face);
            }
        }

        mesh.faceData = faces;
        mesh.numFaces = static_cast<int>(faces.size());
        MeshSystem::updateFaceNormals(mesh);
        setVertexNormalsToPosition(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildAmiga(MeshComponent& mesh, MaterialComponent& material,
                    int lat, int lon) {
        std::vector<VertexData> vertices;
        vertices.resize((lat + 1) * lon);
        mesh.vertexData = vertices;

        for (int i = 0; i <= lat; i++) {
            float theta = i * PI / lat;
            for (int j = 0; j < lon; j++) {
                float phi = j * 2 * PI / lon;
                float x = std::sin(theta) * std::cos(phi);
                float y = std::cos(theta);
                float z = std::sin(theta) * std::sin(phi);
                mesh.vertexData[i * lon + j].vertex = { x, y, z };
                float u = phi / (2 * PI);
                float v = theta / PI;
                mesh.vertexData[i * lon + j].texCoord = { u, v };
            }
        }

        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string mtlPath = "checker-map_tho.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"red", mat});

        mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"white", mat});

        std::vector<FaceData> faces;
        for (int i = 0; i < lat; i++) {
            for (int j = 0; j < lon; j++) {
                int jNext = (j + 1) % lon;
                int row1 = i * lon;
                int row2 = (i + 1) * lon;

                int v1 = row1 + j;
                int v2 = row2 + j;
                int v3 = row1 + jNext;
                int v4 = row2 + jNext;

                std::string color = isRedTile(j / (float)lon, i / (float)lat, lat, lon)
                    ? "red" : "white";

                FaceData face;
                face.face.vertexIndices.push_back(v4);
                face.face.vertexIndices.push_back(v2);
                face.face.vertexIndices.push_back(v1);
                face.face.vertexIndices.push_back(v3);
                face.face.materialKey = color;
                faces.push_back(face);
            }
        }

        mesh.faceData = faces;
        mesh.numFaces = static_cast<int>(faces.size());
        MeshSystem::updateFaceNormals(mesh);
        setVertexNormalsToPosition(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildTetrakis(MeshComponent& mesh, MaterialComponent& material) {
        const float half = 50.f;
        const float axisDist = half * std::sqrt(3.f);

        std::vector<VertexData> vertices;
        for (int xSign : {1, -1}) {
            for (int ySign : {1, -1}) {
                for (int zSign : {1, -1}) {
                    vertices.push_back({ half * xSign, half * ySign, half * zSign });
                }
            }
        }

        vertices.push_back({ axisDist, 0, 0 });
        vertices.push_back({ 0, axisDist, 0 });
        vertices.push_back({ 0, 0, axisDist });
        vertices.push_back({ -axisDist, 0, 0 });
        vertices.push_back({ 0, -axisDist, 0 });
        vertices.push_back({ 0, 0, -axisDist });

        mesh.vertexData = vertices;
        for (auto& vertex : mesh.vertexData) {
            vertex.texCoord = { (vertex.vertex.x / axisDist + 1) / 2,
                                (vertex.vertex.y / axisDist + 1) / 2 };
        }
        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string mtlPath = "checker-map_tho.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x58, 0xfc},
            slib::vec3{0x00, 0x58, 0xfc},
            slib::vec3{0x00, 0x58, 0xfc},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"blue", mat});

        mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"white", mat});

        const uint16_t quads[6][4] = {
            {2, 0, 1, 3},
            {4, 5, 1, 0},
            {2, 6, 4, 0},
            {4, 6, 7, 5},
            {7, 6, 2, 3},
            {1, 5, 7, 3}
        };
        const uint16_t centers[6] = {8, 9, 10, 11, 12, 13};

        std::vector<FaceData> faces;
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 4; j++) {
                FaceData face;
                face.face.vertexIndices.push_back(quads[i][(j + 1) % 4]);
                face.face.vertexIndices.push_back(quads[i][j]);
                face.face.vertexIndices.push_back(centers[i]);
                face.face.materialKey = (j % 2 == 0) ? "blue" : "white";
                faces.push_back(face);
            }
        }

        mesh.faceData = faces;
        mesh.numFaces = static_cast<int>(faces.size());
        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildIcosahedron(MeshComponent& mesh, MaterialComponent& material) {
        const float half = 50.f;
        const float axisDist = half * std::sqrt(3.f);
        const float phi = (1.f + std::sqrt(5.f)) * 0.5f;

        std::vector<VertexData> v = {
            { -1.f,  phi,  0.f },
            {  1.f,  phi,  0.f },
            { -1.f, -phi,  0.f },
            {  1.f, -phi,  0.f },
            {  0.f, -1.f,  phi },
            {  0.f,  1.f,  phi },
            {  0.f, -1.f, -phi },
            {  0.f,  1.f, -phi },
            {  phi,  0.f, -1.f },
            {  phi,  0.f,  1.f },
            { -phi,  0.f, -1.f },
            { -phi,  0.f,  1.f }
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

        mesh.vertexData = v;
        mesh.numVertices = static_cast<int>(v.size());

        MaterialProperties props = MaterialSystem::getMaterialProperties(MaterialType::Light);
        std::string mtlPath = "checker-map_tho.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            props,
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        mat.illum = 1;
        mat.Ke = { props.k_a * 0xff, props.k_a * 0xff, props.k_a * 0xff };
        material.materials.insert({"white", mat});

        const uint16_t F[20][3] = {
            {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
            {1,5,9},  {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
            {3,9,4},  {3,4,2},  {3,2,6},  {3,6,8},  {3,8,9},
            {4,9,5},  {2,4,11}, {6,2,10}, {8,6,7},  {9,8,1}
        };

        std::vector<FaceData> faces;
        faces.reserve(20);
        for (int i = 0; i < 20; ++i) {
            FaceData fd;
            fd.face.vertexIndices.push_back(F[i][0]);
            fd.face.vertexIndices.push_back(F[i][1]);
            fd.face.vertexIndices.push_back(F[i][2]);
            fd.face.materialKey = "white";
            faces.push_back(fd);
        }

        mesh.faceData = faces;
        mesh.numFaces = static_cast<int>(faces.size());
        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

    void buildTest(MeshComponent& mesh, MaterialComponent& material) {
        const float half = 10.f;
        const float axisDist = half * std::sqrt(3.f);

        std::vector<VertexData> vertices;
        vertices.push_back({   axisDist,  axisDist,  axisDist });
        vertices.push_back({  -axisDist,  axisDist,  axisDist });
        vertices.push_back({  -axisDist, -axisDist,  axisDist });
        vertices.push_back({   axisDist, -axisDist,  axisDist });

        vertices.push_back({   axisDist,  axisDist, -axisDist });
        vertices.push_back({  -axisDist,  axisDist, -axisDist });
        vertices.push_back({  -axisDist, -axisDist, -axisDist });
        vertices.push_back({   axisDist, -axisDist, -axisDist });
        mesh.vertexData = vertices;
        mesh.numVertices = static_cast<int>(vertices.size());

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x58, 0xfc},
            slib::vec3{0x00, 0x58, 0xfc},
            slib::vec3{0x00, 0x58, 0xfc}
        );
        material.materials.insert({"blue", mat});

        mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff}
        );
        material.materials.insert({"white", mat});

        std::vector<FaceData> faces;

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

        mesh.faceData = faces;
        mesh.numFaces = static_cast<int>(faces.size());
        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

    bool buildObj(const std::string& filename, MeshComponent& mesh,
                  MaterialComponent& material, TransformComponent& transform) {
        std::filesystem::path filePath(filename);
        std::filesystem::path basePath = filePath.parent_path();

        rapidobj::Result result = rapidobj::ParseFile(filename);
        if (result.error) {
            std::cerr << "Failed to parse OBJ file: " << result.error.code.message() << "\n";
            return false;
        }

        rapidobj::Triangulate(result);
        if (result.error) {
            std::cerr << "Failed to triangulate: " << result.error.code.message() << "\n";
            return false;
        }

        const auto& attrib = result.attributes;
        const auto& shapes = result.shapes;
        const auto& mats = result.materials;

        bool hasLoadedNormals = !attrib.normals.empty();

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string defaultTexturePath = "checker-map_tho.png";

        Material defaultMaterial = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0.1f, 0.1f, 0.1f},
            slib::vec3{0.8f, 0.8f, 0.8f},
            slib::vec3{1.0f, 1.0f, 1.0f},
            std::string(RES_PATH + defaultTexturePath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"default", defaultMaterial});

        for (size_t i = 0; i < mats.size(); ++i) {
            const auto& mat = mats[i];
            Material m{};

            m.Ns = mat.shininess;
            m.Ka = { mat.ambient[0], mat.ambient[1], mat.ambient[2] };
            m.Kd = { mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] };
            m.Ks = { mat.specular[0], mat.specular[1], mat.specular[2] };
            m.Ke = { mat.emission[0], mat.emission[1], mat.emission[2] };
            m.Ni = mat.ior;
            m.d = mat.dissolve;
            m.illum = mat.illum;

            if (!mat.diffuse_texname.empty()) {
                std::filesystem::path texPath = basePath / mat.diffuse_texname;
                if (std::filesystem::exists(texPath)) {
                    m.map_Kd = Texture::loadFromFile(texPath.string());
                } else {
                    std::cerr << "Texture not found: " << texPath << "\n";
                }
            }

            if (!mat.specular_texname.empty()) {
                std::filesystem::path texPath = basePath / mat.specular_texname;
                if (std::filesystem::exists(texPath)) {
                    m.map_Ks = Texture::loadFromFile(texPath.string());
                }
            }

            if (!mat.specular_highlight_texname.empty()) {
                std::filesystem::path texPath = basePath / mat.specular_highlight_texname;
                if (std::filesystem::exists(texPath)) {
                    m.map_Ns = Texture::loadFromFile(texPath.string());
                }
            }

            material.materials[mat.name] = m;
        }

        std::map<std::tuple<int, int, int>, int> vertexMap;
        std::vector<VertexData> finalVertices;
        std::vector<FaceData> faces;

        for (const auto& shape : shapes) {
            const auto& m = shape.mesh;
            size_t indexOffset = 0;
            for (size_t f = 0; f < m.num_face_vertices.size(); ++f) {
                int faceVertCount = m.num_face_vertices[f];

                FaceData faceData;
                int matId = m.material_ids[f];
                if (matId >= 0 && matId < static_cast<int>(mats.size())) {
                    faceData.face.materialKey = mats[matId].name;
                } else {
                    faceData.face.materialKey = "default";
                }

                for (int v = 0; v < faceVertCount; ++v) {
                    const auto& idx = m.indices[indexOffset + v];
                    int posIdx = idx.position_index;
                    int texIdx = idx.texcoord_index;
                    int normIdx = idx.normal_index;

                    auto key = std::make_tuple(posIdx, texIdx, normIdx);

                    int finalIndex;
                    auto it = vertexMap.find(key);
                    if (it != vertexMap.end()) {
                        finalIndex = it->second;
                    } else {
                        VertexData vd;

                        if (posIdx >= 0) {
                            vd.vertex.x = attrib.positions[3 * posIdx + 0];
                            vd.vertex.y = attrib.positions[3 * posIdx + 1];
                            vd.vertex.z = attrib.positions[3 * posIdx + 2];
                        }

                        if (texIdx >= 0) {
                            vd.texCoord.x = attrib.texcoords[2 * texIdx + 0];
                            vd.texCoord.y = attrib.texcoords[2 * texIdx + 1];
                        } else {
                            vd.texCoord = { 0.0f, 0.0f };
                        }

                        if (normIdx >= 0) {
                            vd.normal.x = attrib.normals[3 * normIdx + 0];
                            vd.normal.y = attrib.normals[3 * normIdx + 1];
                            vd.normal.z = attrib.normals[3 * normIdx + 2];
                        } else {
                            vd.normal = { 0.0f, 0.0f, 0.0f };
                        }

                        finalIndex = static_cast<int>(finalVertices.size());
                        finalVertices.push_back(vd);
                        vertexMap[key] = finalIndex;
                    }

                    faceData.face.vertexIndices.push_back(finalIndex);
                }

                faces.push_back(faceData);
                indexOffset += faceVertCount;
            }
        }

        if (attrib.texcoords.empty() && !finalVertices.empty()) {
            float x_min = finalVertices[0].vertex.x;
            float y_min = finalVertices[0].vertex.y;
            float x_max = finalVertices[0].vertex.x;
            float y_max = finalVertices[0].vertex.y;

            for (const auto& v : finalVertices) {
                x_min = std::min(x_min, v.vertex.x);
                y_min = std::min(y_min, v.vertex.y);
                x_max = std::max(x_max, v.vertex.x);
                y_max = std::max(y_max, v.vertex.y);
            }

            float rangeX = (x_max - x_min);
            float rangeY = (y_max - y_min);
            if (rangeX < 0.0001f) rangeX = 1.0f;
            if (rangeY < 0.0001f) rangeY = 1.0f;

            for (auto& v : finalVertices) {
                v.texCoord.x = (v.vertex.x - x_min) / rangeX;
                v.texCoord.y = (v.vertex.y - y_min) / rangeY;
            }
        }

        std::cout << "Loaded OBJ: " << filename << "\n";
        std::cout << "  Positions: " << attrib.positions.size() / 3 << "\n";
        std::cout << "  Texture coords: " << attrib.texcoords.size() / 2 << "\n";
        std::cout << "  Normals: " << attrib.normals.size() / 3
                  << (hasLoadedNormals ? " (using file normals)" : " (will calculate)") << "\n";
        std::cout << "  Final vertices: " << finalVertices.size() << "\n";
        std::cout << "  Faces: " << faces.size() << "\n";
        std::cout << "  Materials: " << material.materials.size() << "\n";

        mesh.vertexData = std::move(finalVertices);
        mesh.faceData = std::move(faces);
        mesh.numVertices = static_cast<int>(mesh.vertexData.size());
        mesh.numFaces = static_cast<int>(mesh.faceData.size());

        MeshSystem::updateFaceNormals(mesh);
        if (!hasLoadedNormals) {
            MeshSystem::updateVertexNormals(mesh);
        }
        MeshSystem::updateRadius(mesh);
        TransformSystem::scaleToRadius(transform, mesh.radius, 400.0f);

        return hasLoadedNormals;
    }

    void buildAsc(const std::string& filename, MeshComponent& mesh,
                  MaterialComponent& material) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file.\n";
            return;
        }

        std::string line;
        bool readingVertices = false;
        bool readingFaces = false;

        std::vector<VertexData> vertices;
        std::vector<FaceData> faces;

        MaterialProperties properties = MaterialSystem::getMaterialProperties(MaterialType::Metal);
        std::string mtlPath = "checker-map_tho.png";

        Material mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0x00, 0x58, 0xfc},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"blue", mat});

        mat = MaterialSystem::initDefaultMaterial(
            properties,
            slib::vec3{0x00, 0x00, 0x00},
            slib::vec3{0xff, 0xff, 0xff},
            slib::vec3{0xff, 0xff, 0xff},
            std::string(RES_PATH + mtlPath),
            TextureFilter::NEIGHBOUR
        );
        material.materials.insert({"white", mat});

        while (std::getline(file, line)) {
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty())
                continue;

            if (line.find("Vertex list:") != std::string::npos) {
                readingVertices = true;
                readingFaces = false;
                continue;
            }

            if (line.find("Face list:") != std::string::npos) {
                readingVertices = false;
                readingFaces = true;
                continue;
            }

            if (readingVertices) {
                if (line.find("Vertex") != std::string::npos) {
                    VertexData vertexData;
                    std::regex vertexRegex(R"(Vertex\s+\d+:\s+X:\s+([-.\dEe]+)\s+Y:\s+([-.\dEe]+)\s+Z:\s+([-.\dEe]+))");
                    std::smatch match;

                    if (std::regex_search(line, match, vertexRegex)) {
                        vertexData.vertex.x = std::stof(match[1]);
                        vertexData.vertex.y = std::stof(match[2]);
                        vertexData.vertex.z = std::stof(match[3]);
                        vertices.push_back(vertexData);
                    }
                }
            }

            if (readingFaces) {
                if (line.find("Face") != std::string::npos) {
                    std::regex faceRegex(R"(Face\s+\d+:\s+A:(\d+)\s+B:(\d+)\s+C:(\d+))");
                    std::smatch match;

                    if (std::regex_search(line, match, faceRegex)) {
                        FaceData faceData;
                        faceData.face.vertexIndices.push_back(std::stoi(match[1]));
                        faceData.face.vertexIndices.push_back(std::stoi(match[2]));
                        faceData.face.vertexIndices.push_back(std::stoi(match[3]));
                        faceData.face.materialKey = "blue";
                        faces.push_back(faceData);
                    }
                }
            }
        }

        file.close();

        int num_vertex = static_cast<int>(vertices.size());
        int num_faces = static_cast<int>(faces.size());

        std::cout << "Total vertices: " << num_vertex << "\n";
        std::cout << "Total faces: " << num_faces << "\n";

        mesh.vertexData = vertices;

        float x_min = 0.0f;
        float y_min = 0.0f;
        float x_max = 0.0f;
        float y_max = 0.0f;

        for (auto& vertex : mesh.vertexData) {
            if (vertex.vertex.x < x_min) x_min = vertex.vertex.x;
            if (vertex.vertex.y < y_min) y_min = vertex.vertex.y;
            if (vertex.vertex.x > x_max) x_max = vertex.vertex.x;
            if (vertex.vertex.y > y_max) y_max = vertex.vertex.y;
        }

        for (auto& vertex : mesh.vertexData) {
            vertex.texCoord.x = (vertex.vertex.x - x_min) / (x_max - x_min);
            vertex.texCoord.y = (vertex.vertex.y - y_min) / (y_max - y_min);
        }

        mesh.faceData = faces;
        mesh.numVertices = num_vertex;
        mesh.numFaces = num_faces;

        MeshSystem::updateFaceNormals(mesh);
        MeshSystem::updateVertexNormals(mesh);
        MeshSystem::updateRadius(mesh);
    }

} // namespace PrefabFactory
