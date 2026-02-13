#include <iostream>
#include <cmath>
#include <filesystem>
#include <map>
#include <tuple>
#include "objLoader.hpp"
#include "../material.hpp"
#include "../ecs/MeshSystem.hpp"

#include <rapidobj/rapidobj.hpp>

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
    if (mesh) {
        MeshSystem::markBoundsDirty(*mesh);
    }
}

void ObjLoader::loadVertices(const std::string& filename) {
    std::filesystem::path filePath(filename);
    std::filesystem::path basePath = filePath.parent_path();

    // Parse OBJ file with rapidobj
    rapidobj::Result result = rapidobj::ParseFile(filename);

    if (result.error) {
        std::cerr << "Failed to parse OBJ file: " << result.error.code.message() << "\n";
        return;
    }

    // Triangulate the mesh (rapidobj can have n-gons)
    rapidobj::Triangulate(result);

    if (result.error) {
        std::cerr << "Failed to triangulate: " << result.error.code.message() << "\n";
        return;
    }

    const auto& attrib = result.attributes;
    const auto& shapes = result.shapes;
    const auto& mats = result.materials;

    // Track if normals were loaded from the file
    hasLoadedNormals = !attrib.normals.empty();

    // Create default material
    MaterialProperties properties = getMaterialProperties(MaterialType::Metal);
    std::string defaultTexturePath = "checker-map_tho.png";

    Material defaultMaterial{};
    defaultMaterial.Ka = { properties.k_a * 0.1f, properties.k_a * 0.1f, properties.k_a * 0.1f };
    defaultMaterial.Kd = { properties.k_d * 0.8f, properties.k_d * 0.8f, properties.k_d * 0.8f };
    defaultMaterial.Ks = { properties.k_s * 1.0f, properties.k_s * 1.0f, properties.k_s * 1.0f };
    defaultMaterial.map_Kd = Texture::loadFromFile(std::string(RES_PATH + defaultTexturePath));
    defaultMaterial.map_Kd.setFilter(TextureFilter::NEIGHBOUR);
    defaultMaterial.Ns = properties.shininess;
    mesh->materials.insert({"default", defaultMaterial});

    // Load materials from rapidobj
    for (size_t i = 0; i < mats.size(); ++i) {
        const auto& mat = mats[i];
        Material material{};

        material.Ns = mat.shininess;
        material.Ka = { mat.ambient[0], mat.ambient[1], mat.ambient[2] };
        material.Kd = { mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] };
        material.Ks = { mat.specular[0], mat.specular[1], mat.specular[2] };
        material.Ke = { mat.emission[0], mat.emission[1], mat.emission[2] };
        material.Ni = mat.ior;
        material.d = mat.dissolve;
        material.illum = mat.illum;

        // Load diffuse texture (map_Kd)
        if (!mat.diffuse_texname.empty()) {
            std::filesystem::path texPath = basePath / mat.diffuse_texname;
            if (std::filesystem::exists(texPath)) {
                material.map_Kd = Texture::loadFromFile(texPath.string());
            } else {
                std::cerr << "Texture not found: " << texPath << "\n";
            }
        }

        // Load specular texture (map_Ks)
        if (!mat.specular_texname.empty()) {
            std::filesystem::path texPath = basePath / mat.specular_texname;
            if (std::filesystem::exists(texPath)) {
                material.map_Ks = Texture::loadFromFile(texPath.string());
            }
        }

        // Load specular highlight texture (map_Ns)
        if (!mat.specular_highlight_texname.empty()) {
            std::filesystem::path texPath = basePath / mat.specular_highlight_texname;
            if (std::filesystem::exists(texPath)) {
                material.map_Ns = Texture::loadFromFile(texPath.string());
            }
        }

        mesh->materials[mat.name] = material;
    }

    // Map to track unique vertex combinations (position_index, texcoord_index, normal_index) -> final index
    std::map<std::tuple<int, int, int>, int> vertexMap;
    std::vector<VertexData> finalVertices;
    std::vector<FaceData> faces;

    // Process all shapes
    for (const auto& shape : shapes) {
        const auto& mesh = shape.mesh;

        // Process faces (triangulated, so every 3 indices form a face)
        size_t indexOffset = 0;
        for (size_t f = 0; f < mesh.num_face_vertices.size(); ++f) {
            int faceVertCount = mesh.num_face_vertices[f];

            FaceData faceData;

            // Get material for this face
            int matId = mesh.material_ids[f];
            if (matId >= 0 && matId < static_cast<int>(mats.size())) {
                faceData.face.materialKey = mats[matId].name;
            } else {
                faceData.face.materialKey = "default";
            }

            // Process each vertex in the face
            for (int v = 0; v < faceVertCount; ++v) {
                const auto& idx = mesh.indices[indexOffset + v];

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

                    // Position
                    if (posIdx >= 0) {
                        vd.vertex.x = attrib.positions[3 * posIdx + 0];
                        vd.vertex.y = attrib.positions[3 * posIdx + 1];
                        vd.vertex.z = attrib.positions[3 * posIdx + 2];
                    }

                    // Texture coordinates
                    if (texIdx >= 0) {
                        vd.texCoord.x = attrib.texcoords[2 * texIdx + 0];
                        vd.texCoord.y = attrib.texcoords[2 * texIdx + 1];
                    } else {
                        vd.texCoord = { 0.0f, 0.0f };
                    }

                    // Normal
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

    // If no texture coordinates were provided, generate planar mapping
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
    std::cout << "  Normals: " << attrib.normals.size() / 3 << (hasLoadedNormals ? " (using file normals)" : " (will calculate)") << "\n";
    std::cout << "  Final vertices: " << finalVertices.size() << "\n";
    std::cout << "  Faces: " << faces.size() << "\n";
    std::cout << "  Materials: " << mesh->materials.size() << "\n";

    mesh->vertexData = std::move(finalVertices);
    mesh->faceData = std::move(faces);
    mesh->numVertices = static_cast<int>(mesh->vertexData.size());
    mesh->numFaces = static_cast<int>(mesh->faceData.size());
}

void ObjLoader::loadFaces() {
    // Faces are already loaded in loadVertices
}

void ObjLoader::loadVertices() {
    // Empty - actual loading done in loadVertices(filename)
}
