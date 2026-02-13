#pragma once
#include <map>
#include <string>
#include <vector>
#include "../material.hpp"
#include "../slib.hpp"

struct VertexData {
    slib::vec3 vertex;
    slib::vec3 normal;
    slib::vec2 texCoord;
};

typedef struct Face {
    std::vector<int> vertexIndices; // For wireframe rendering
    std::string materialKey;
} Face;

struct FaceData {
    Face face;
    slib::vec3 faceNormal;
};

struct MeshComponent {
    std::vector<VertexData> vertexData;
    std::vector<FaceData> faceData;
    std::map<std::string, Material> materials;
    int numVertices = 0;
    int numFaces = 0;
    slib::vec3 minCoord{};
    slib::vec3 maxCoord{};
    bool boundsDirty = true;
};
