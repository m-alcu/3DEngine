#include <iostream>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <regex>
#include <cstdint>
#include <filesystem>
#include "objLoader.hpp"
#include "../material.hpp"

void ObjLoader::setup(const std::string& filename) {
    std::filesystem::path filePath(filename);
    this->name = filePath.stem().string();
    loadVertices(filename);
    loadFaces();
    calculateNormals();
    calculateVertexNormals();
    calculateMinMaxCoords();
    this->scaleToRadius(400.0f);
}

void ObjLoader::loadVertices(const std::string& filename) {
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

    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);

    std::string mtlPath = "checker-map_tho.png";

    Material material{};
    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0x00, properties.k_d * 0x58, properties.k_d * 0xfc }; 
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    material.map_Kd.setFilter(TextureFilter::NEIGHBOUR);
    material.Ns = properties.shininess;
    materials.insert({"blue", material});

    material.Ka = { properties.k_a * 0x00, properties.k_a * 0x00, properties.k_a * 0x00 };
    material.Kd = { properties.k_d * 0xff, properties.k_d * 0xff, properties.k_d * 0xff };
    material.Ks = { properties.k_s * 0xff, properties.k_s * 0xff, properties.k_s * 0xff };
    material.map_Kd = DecodePng(std::string(RES_PATH + mtlPath).c_str());
    material.map_Kd.setFilter(TextureFilter::NEIGHBOUR);
    material.Ns = properties.shininess;
    materials.insert({"white", material});  

    while (std::getline(file, line)) {
        // Remove leading/trailing spaces
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines
        if (line.empty())
            continue;


        if (line.find("v") != std::string::npos) {
            // Example line: v -43.592037 7.219297 -21.717901
            VertexData vertexData;
            std::regex vertexRegex(R"(^v\s+([-+\.\dEe]+)\s+([-+\.\dEe]+)\s+([-+\.\dEe]+))");
            std::smatch match;

            if (std::regex_search(line, match, vertexRegex)) {
                vertexData.vertex.x = std::stof(match[1]);
                vertexData.vertex.y = std::stof(match[2]);
                vertexData.vertex.z = std::stof(match[3]);

                vertices.push_back(vertexData);
            }
        }

        if (line[0] == 'f' && (line[1] == ' ' || line[1] == '\t')) {
            // Parse face line - handles all OBJ formats:
            // f v1 v2 v3 ... (vertex only)
            // f v1/vt1 v2/vt2 v3/vt3 ... (vertex / texture coords)
            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ... (vertex / texture coords / normals)
            // f v1//vn1 v2//vn2 v3//vn3 ... (vertex / normals)
            FaceData faceData;
            std::istringstream iss(line.substr(2)); // Skip "f "
            std::string token;

            while (iss >> token) {
                // Extract vertex index (first number before any '/')
                size_t slashPos = token.find('/');
                std::string vertexStr = (slashPos != std::string::npos)
                    ? token.substr(0, slashPos)
                    : token;

                if (!vertexStr.empty()) {
                    int vertexIndex = std::stoi(vertexStr) - 1; // OBJ indices are 1-based
                    faceData.face.vertexIndices.push_back(vertexIndex);
                }
            }

            if (faceData.face.vertexIndices.size() >= 3) {
                faceData.face.materialKey = "blue";
                faces.push_back(faceData);
            }
        }
    }

    file.close();

    // Calculate total number of vertices and faces
    int num_vertex = vertices.size();
    int num_faces = faces.size();

    std::cout << "Total vertices: " << num_vertex << "\n";
    std::cout << "Total faces: " << num_faces << "\n";

    // Store vertices and faces in the class members
    ObjLoader::vertexData = vertices;

    float x_min = 0.0f;
    float y_min = 0.0f;
    float x_max = 0.0f;
    float y_max = 0.0f;

    for (auto& vertex : ObjLoader::vertexData) {
        // Calculate min and max for x and y coordinates
        if (vertex.vertex.x < x_min) x_min = vertex.vertex.x;
        if (vertex.vertex.y < y_min) y_min = vertex.vertex.y;
        if (vertex.vertex.x > x_max) x_max = vertex.vertex.x;
        if (vertex.vertex.y > y_max) y_max = vertex.vertex.y;
    }

    for (auto& vertex : ObjLoader::vertexData) {
        // Set texture coordinates based on the min and max values
        vertex.texCoord.x = (vertex.vertex.x - x_min) / (x_max - x_min);
        vertex.texCoord.y = (vertex.vertex.y - y_min) / (y_max - y_min);
    }

    ObjLoader::faceData = faces;
    ObjLoader::numVertices = num_vertex;
    ObjLoader::numFaces = num_faces;
}

void ObjLoader::loadFaces() {
    calculateNormals();
    calculateVertexNormals();
}

void ObjLoader::loadVertices() {
}
