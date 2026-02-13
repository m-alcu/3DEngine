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
#include "ascLoader.hpp"
#include "../material.hpp"
#include "../ecs/MeshSystem.hpp"
#include "../ecs/MaterialSystem.hpp"

void AscLoader::setup(const std::string& filename) {
    std::filesystem::path filePath(filename);
    this->name = filePath.stem().string();
    loadVertices(filename);
    loadFaces();
    calculateFaceNormals();
    calculateVertexNormals();
    calculateMinMaxCoords();
    if (mesh) {
        MeshSystem::markBoundsDirty(*mesh);
    }
}

void AscLoader::loadVertices(const std::string& filename) {
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

    Material material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0x00, 0x00, 0x00},
        slib::vec3{0x00, 0x58, 0xfc},
        slib::vec3{0xff, 0xff, 0xff},
        std::string(RES_PATH + mtlPath),
        TextureFilter::NEIGHBOUR
    );
    materialComponent->materials.insert({"blue", material});

    material = MaterialSystem::initDefaultMaterial(
        properties,
        slib::vec3{0x00, 0x00, 0x00},
        slib::vec3{0xff, 0xff, 0xff},
        slib::vec3{0xff, 0xff, 0xff},
        std::string(RES_PATH + mtlPath),
        TextureFilter::NEIGHBOUR
    );
    materialComponent->materials.insert({"white", material});          

    while (std::getline(file, line)) {
        // Remove leading/trailing spaces
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines
        if (line.empty())
            continue;

        // Detect start of vertex list
        if (line.find("Vertex list:") != std::string::npos) {
            readingVertices = true;
            readingFaces = false;
            continue;
        }

        // Detect start of face list
        if (line.find("Face list:") != std::string::npos) {
            readingVertices = false;
            readingFaces = true;
            continue;
        }

        if (readingVertices) {
            if (line.find("Vertex") != std::string::npos) {
                // Example line: Vertex 0:  X: -95     Y: 0     Z: 0
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
                // Example line: Face 0:    A:0 B:1 C:2 AB:1 BC:1 CA:0
                FaceData faceData;
                std::regex faceRegex(R"(Face\s+\d+:\s+A:(\d+)\s+B:(\d+)\s+C:(\d+))");
                std::smatch match;

                if (std::regex_search(line, match, faceRegex)) {

                    FaceData faceData;

					faceData.face.vertexIndices.push_back(std::stoi(match[1]));
					faceData.face.vertexIndices.push_back(std::stoi(match[2]));
					faceData.face.vertexIndices.push_back(std::stoi(match[3]));
                    faceData.face.materialKey = "blue"; // Default material key
                    faces.push_back(faceData);
                }
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
    mesh->vertexData = vertices;

    float x_min = 0.0f;
    float y_min = 0.0f;
    float x_max = 0.0f;
    float y_max = 0.0f;

    for (auto& vertex : mesh->vertexData) {
        // Calculate min and max for x and y coordinates
        if (vertex.vertex.x < x_min) x_min = vertex.vertex.x;
        if (vertex.vertex.y < y_min) y_min = vertex.vertex.y;
        if (vertex.vertex.x > x_max) x_max = vertex.vertex.x;
        if (vertex.vertex.y > y_max) y_max = vertex.vertex.y;
    }

    for (auto& vertex : mesh->vertexData) {
        // Set texture coordinates based on the min and max values
        vertex.texCoord.x = (vertex.vertex.x - x_min) / (x_max - x_min);
        vertex.texCoord.y = (vertex.vertex.y - y_min) / (y_max - y_min);
    }

    mesh->faceData = faces;
    mesh->numVertices = num_vertex;
    mesh->numFaces = num_faces;
}

void AscLoader::loadFaces() {
    calculateFaceNormals();
    calculateVertexNormals();
}

void AscLoader::loadVertices() {
}
