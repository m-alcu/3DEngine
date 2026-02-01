#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <tuple>
#include <functional>
#include <cstdint>
#include <filesystem>
#include "objLoader.hpp"
#include "../material.hpp"

// Structure to hold indices for a face vertex (v/vt/vn)
struct FaceVertex {
    int vertexIndex = -1;
    int texCoordIndex = -1;
    int normalIndex = -1;
};

// Parse a single face vertex token like "1/2/3" or "1//3" or "1/2" or "1"
static FaceVertex parseFaceVertex(const std::string& token) {
    FaceVertex fv;
    std::vector<std::string> parts;
    std::stringstream ss(token);
    std::string part;

    while (std::getline(ss, part, '/')) {
        parts.push_back(part);
    }

    if (parts.size() >= 1 && !parts[0].empty()) {
        fv.vertexIndex = std::stoi(parts[0]) - 1; // OBJ indices are 1-based
    }
    if (parts.size() >= 2 && !parts[1].empty()) {
        fv.texCoordIndex = std::stoi(parts[1]) - 1;
    }
    if (parts.size() >= 3 && !parts[2].empty()) {
        fv.normalIndex = std::stoi(parts[2]) - 1;
    }

    return fv;
}

// Parse MTL file and return a map of material name to Material
static std::map<std::string, Material> parseMtlFile(const std::string& mtlPath,
                                                     std::function<Texture(const char*)> decodePng) {
    std::map<std::string, Material> materials;

    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open MTL file: " << mtlPath << "\n";
        return materials;
    }

    std::filesystem::path basePath = std::filesystem::path(mtlPath).parent_path();

    std::string line;
    std::string currentMaterialName;
    Material currentMaterial;
    bool hasMaterial = false;

    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "newmtl") {
            // Save previous material if exists
            if (hasMaterial) {
                materials[currentMaterialName] = currentMaterial;
            }
            iss >> currentMaterialName;
            currentMaterial = Material{};
            hasMaterial = true;
        }
        else if (keyword == "Ns") {
            iss >> currentMaterial.Ns;
        }
        else if (keyword == "Ka") {
            iss >> currentMaterial.Ka.x >> currentMaterial.Ka.y >> currentMaterial.Ka.z;
        }
        else if (keyword == "Kd") {
            iss >> currentMaterial.Kd.x >> currentMaterial.Kd.y >> currentMaterial.Kd.z;
        }
        else if (keyword == "Ks") {
            iss >> currentMaterial.Ks.x >> currentMaterial.Ks.y >> currentMaterial.Ks.z;
        }
        else if (keyword == "Ke") {
            iss >> currentMaterial.Ke.x >> currentMaterial.Ke.y >> currentMaterial.Ke.z;
        }
        else if (keyword == "Ni") {
            iss >> currentMaterial.Ni;
        }
        else if (keyword == "d") {
            iss >> currentMaterial.d;
        }
        else if (keyword == "illum") {
            iss >> currentMaterial.illum;
        }
        else if (keyword == "map_Kd") {
            std::string texturePath;
            iss >> texturePath;
            std::filesystem::path fullPath = basePath / texturePath;
            if (std::filesystem::exists(fullPath)) {
                currentMaterial.map_Kd = decodePng(fullPath.string().c_str());
            } else {
                std::cerr << "Texture not found: " << fullPath << "\n";
            }
        }
        else if (keyword == "map_Ks") {
            std::string texturePath;
            iss >> texturePath;
            std::filesystem::path fullPath = basePath / texturePath;
            if (std::filesystem::exists(fullPath)) {
                currentMaterial.map_Ks = decodePng(fullPath.string().c_str());
            }
        }
        else if (keyword == "map_Ns") {
            std::string texturePath;
            iss >> texturePath;
            std::filesystem::path fullPath = basePath / texturePath;
            if (std::filesystem::exists(fullPath)) {
                currentMaterial.map_Ns = decodePng(fullPath.string().c_str());
            }
        }
    }

    // Save last material
    if (hasMaterial) {
        materials[currentMaterialName] = currentMaterial;
    }

    file.close();
    return materials;
}

void ObjLoader::setup(const std::string& filename) {
    std::filesystem::path filePath(filename);
    this->name = filePath.stem().string();
    loadVertices(filename);
    loadFaces();
    calculateFaceNormals();
    // Only calculate vertex normals if they weren't provided in the OBJ file
    if (!hasLoadedNormals) {
        calculateVertexNormals();
    }
    calculateMinMaxCoords();
    this->scaleToRadius(400.0f);
}

void ObjLoader::loadVertices(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << "\n";
        return;
    }

    std::filesystem::path filePath(filename);
    std::filesystem::path basePath = filePath.parent_path();

    std::string line;

    // Temporary storage for raw OBJ data
    std::vector<slib::vec3> rawVertices;
    std::vector<slib::vec2> rawTexCoords;
    std::vector<slib::vec3> rawNormals;
    std::vector<FaceData> faces;

    // Map to track unique vertex combinations (v/vt/vn) -> final index
    std::map<std::tuple<int, int, int>, int> vertexMap;
    std::vector<VertexData> finalVertices;

    std::string currentMaterialName = "default";

    // Create default material with default texture
    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);
    std::string defaultTexturePath = "checker-map_tho.png";

    Material defaultMaterial{};
    defaultMaterial.Ka = { properties.k_a * 0.1f, properties.k_a * 0.1f, properties.k_a * 0.1f };
    defaultMaterial.Kd = { properties.k_d * 0.8f, properties.k_d * 0.8f, properties.k_d * 0.8f };
    defaultMaterial.Ks = { properties.k_s * 1.0f, properties.k_s * 1.0f, properties.k_s * 1.0f };
    defaultMaterial.map_Kd = DecodePng(std::string(RES_PATH + defaultTexturePath).c_str());
    defaultMaterial.map_Kd.setFilter(TextureFilter::NEIGHBOUR);
    defaultMaterial.Ns = properties.shininess;
    materials.insert({"default", defaultMaterial});

    while (std::getline(file, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "mtllib") {
            // Load material library
            std::string mtlFilename;
            iss >> mtlFilename;
            std::filesystem::path mtlPath = basePath / mtlFilename;

            if (std::filesystem::exists(mtlPath)) {
                auto loadedMaterials = parseMtlFile(mtlPath.string(),
                    [this](const char* path) { return this->DecodePng(path); });
                for (auto& [name, mat] : loadedMaterials) {
                    materials[name] = mat;
                }
                std::cout << "Loaded " << loadedMaterials.size() << " materials from " << mtlFilename << "\n";
            } else {
                std::cerr << "MTL file not found: " << mtlPath << "\n";
            }
        }
        else if (keyword == "usemtl") {
            iss >> currentMaterialName;
            // If material doesn't exist, use default
            if (materials.find(currentMaterialName) == materials.end()) {
                std::cerr << "Material not found: " << currentMaterialName << ", using default\n";
                currentMaterialName = "default";
            }
        }
        else if (keyword == "v") {
            // Vertex position
            slib::vec3 v;
            iss >> v.x >> v.y >> v.z;
            rawVertices.push_back(v);
        }
        else if (keyword == "vt") {
            // Texture coordinate
            slib::vec2 vt;
            iss >> vt.x >> vt.y;
            // OBJ uses bottom-left origin, flip Y if needed
            rawTexCoords.push_back(vt);
        }
        else if (keyword == "vn") {
            // Vertex normal
            slib::vec3 vn;
            iss >> vn.x >> vn.y >> vn.z;
            rawNormals.push_back(vn);
        }
        else if (keyword == "f") {
            // Face - parse all vertex tokens
            std::vector<FaceVertex> faceVertices;
            std::string token;

            while (iss >> token) {
                faceVertices.push_back(parseFaceVertex(token));
            }

            if (faceVertices.size() < 3) continue;

            // Build face with final vertex indices
            FaceData faceData;
            faceData.face.materialKey = currentMaterialName;

            for (const auto& fv : faceVertices) {
                // Handle negative indices (relative to end of list)
                int vIdx = fv.vertexIndex;
                int vtIdx = fv.texCoordIndex;
                int vnIdx = fv.normalIndex;

                if (vIdx < 0) vIdx = static_cast<int>(rawVertices.size()) + vIdx + 1;
                if (vtIdx < 0 && fv.texCoordIndex != -1) vtIdx = static_cast<int>(rawTexCoords.size()) + vtIdx + 1;
                if (vnIdx < 0 && fv.normalIndex != -1) vnIdx = static_cast<int>(rawNormals.size()) + vnIdx + 1;

                auto key = std::make_tuple(vIdx, vtIdx, vnIdx);

                int finalIndex;
                auto it = vertexMap.find(key);
                if (it != vertexMap.end()) {
                    finalIndex = it->second;
                } else {
                    // Create new vertex
                    VertexData vd;

                    if (vIdx >= 0 && vIdx < static_cast<int>(rawVertices.size())) {
                        vd.vertex = rawVertices[vIdx];
                    }

                    if (vtIdx >= 0 && vtIdx < static_cast<int>(rawTexCoords.size())) {
                        vd.texCoord = rawTexCoords[vtIdx];
                    } else {
                        // Generate planar UV if no texture coords
                        vd.texCoord = { 0.0f, 0.0f };
                    }

                    if (vnIdx >= 0 && vnIdx < static_cast<int>(rawNormals.size())) {
                        vd.normal = rawNormals[vnIdx];
                    } else {
                        // Normal will be calculated later
                        vd.normal = { 0.0f, 0.0f, 0.0f };
                    }

                    finalIndex = static_cast<int>(finalVertices.size());
                    finalVertices.push_back(vd);
                    vertexMap[key] = finalIndex;
                }

                faceData.face.vertexIndices.push_back(finalIndex);
            }

            faces.push_back(faceData);
        }
    }

    file.close();

    // If no texture coordinates were provided, generate planar mapping
    if (rawTexCoords.empty() && !finalVertices.empty()) {
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

    // Track if normals were loaded from the file
    hasLoadedNormals = !rawNormals.empty();

    std::cout << "Loaded OBJ: " << filename << "\n";
    std::cout << "  Raw vertices: " << rawVertices.size() << "\n";
    std::cout << "  Texture coords: " << rawTexCoords.size() << "\n";
    std::cout << "  Normals: " << rawNormals.size() << (hasLoadedNormals ? " (using file normals)" : " (will calculate)") << "\n";
    std::cout << "  Final vertices: " << finalVertices.size() << "\n";
    std::cout << "  Faces: " << faces.size() << "\n";
    std::cout << "  Materials: " << materials.size() << "\n";

    vertexData = std::move(finalVertices);
    faceData = std::move(faces);
    numVertices = static_cast<int>(vertexData.size());
    numFaces = static_cast<int>(faceData.size());
}

void ObjLoader::loadFaces() {
    // Faces are already loaded in loadVertices
    // This is called by base class setup, just recalculate normals
}

void ObjLoader::loadVertices() {
    // Empty - actual loading done in loadVertices(filename)
}
