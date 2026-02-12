#include <iostream>
#include <math.h>
#include "solid.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/TransformSystem.hpp"
#include "../vendor/stb/stb_image.h"

Solid::Solid() {
}

void Solid::calculateTransformMat() {
    TransformSystem::updateTransform(*transform);
}

void Solid::calculateFaceNormals() {
    if (!mesh) return;

    for (int i = 0; i < mesh->numFaces; i++) {
        const Face &face = mesh->faceData[i].face;
        const size_t n = face.vertexIndices.size();

        // Newell's method: works for any polygon (tri, quad, n-gon)
        // and handles degenerate edges gracefully
        // https://every-algorithm.github.io/2024/03/06/newells_algorithm.html
        slib::vec3 normal = {0.0f, 0.0f, 0.0f};

        for (size_t j = 0; j < n; ++j) {
            const slib::vec3& curr = mesh->vertexData[face.vertexIndices[j]].vertex;
            const slib::vec3& next = mesh->vertexData[face.vertexIndices[(j + 1) % n]].vertex;

            normal.x += (curr.y - next.y) * (curr.z + next.z);
            normal.y += (curr.z - next.z) * (curr.x + next.x);
            normal.z += (curr.x - next.x) * (curr.y + next.y);
        }
        mesh->faceData[i].faceNormal = smath::normalize(normal);
    }
}

void Solid::calculateVertexNormals() {
    if (!mesh) return;

    for (int i = 0; i < mesh->numVertices; i++) {
        slib::vec3 vertexNormal = { 0, 0, 0 };
        for(int j = 0; j < mesh->numFaces; j++) {

            for (int vi : mesh->faceData[j].face.vertexIndices) {
                // guard in case your data can contain bad indices
                if (vi == i) {
                    vertexNormal += mesh->faceData[j].faceNormal;
                }
            }        
        }
        mesh->vertexData[i].normal = smath::normalize(vertexNormal);
    }

}

void Solid::calculateMinMaxCoords() {
    if (!mesh || mesh->numVertices == 0) {
        if (mesh) {
            mesh->minCoord = {0.0f, 0.0f, 0.0f};
            mesh->maxCoord = {0.0f, 0.0f, 0.0f};
        }
        return;
    }

    // Initialize with the first vertex
    mesh->minCoord = mesh->vertexData[0].vertex;
    mesh->maxCoord = mesh->vertexData[0].vertex;

    // Iterate through all vertices to find min and max coordinates
    for (int i = 1; i < mesh->numVertices; i++) {
        const slib::vec3& v = mesh->vertexData[i].vertex;

        // Update minimum coordinates
        if (v.x < mesh->minCoord.x) mesh->minCoord.x = v.x;
        if (v.y < mesh->minCoord.y) mesh->minCoord.y = v.y;
        if (v.z < mesh->minCoord.z) mesh->minCoord.z = v.z;

        // Update maximum coordinates
        if (v.x > mesh->maxCoord.x) mesh->maxCoord.x = v.x;
        if (v.y > mesh->maxCoord.y) mesh->maxCoord.y = v.y;
        if (v.z > mesh->maxCoord.z) mesh->maxCoord.z = v.z;
    }
}

float Solid::getBoundingRadius() const {
    if (!mesh) return 0.0f;
    slib::vec3 center{(mesh->minCoord.x + mesh->maxCoord.x) * 0.5f,
                      (mesh->minCoord.y + mesh->maxCoord.y) * 0.5f,
                      (mesh->minCoord.z + mesh->maxCoord.z) * 0.5f};
    slib::vec3 halfDiag{mesh->maxCoord.x - center.x, mesh->maxCoord.y - center.y,
                        mesh->maxCoord.z - center.z};
    return smath::distance(halfDiag);
}

slib::vec3 Solid::getWorldCenter() const {
    if (!mesh) return {0.0f, 0.0f, 0.0f};
    return TransformSystem::getWorldCenter(*transform, mesh->minCoord, mesh->maxCoord);
}

void Solid::updateWorldBounds(slib::vec3& worldBoundMin, slib::vec3& worldBoundMax) const {
    if (!mesh) return;
    TransformSystem::updateWorldBounds(*transform, mesh->minCoord, mesh->maxCoord, worldBoundMin, worldBoundMax);
}

void Solid::scaleToRadius(float targetRadius) {
    TransformSystem::scaleToRadius(*transform, getBoundingRadius(), targetRadius);
}

// Function returning MaterialProperties struct
MaterialProperties Solid::getMaterialProperties(MaterialType type) {
    switch (type) {
        case MaterialType::Rubber:   return {0.1f, 0.2f, 0.5f, 2};  // Low specular, moderate ambient, height diffuse
        case MaterialType::Plastic:  return {0.3f, 0.2f, 0.6f, 2};
        case MaterialType::Wood:     return {0.2f, 0.3f, 0.7f, 2};
        case MaterialType::Marble:   return {0.4f, 0.4f, 0.8f, 2};
        case MaterialType::Glass:    return {0.6f, 0.1f, 0.2f, 2};  // High specular, low ambient, low diffuse
        case MaterialType::Metal:    return {0.4f, 0.2f, 0.4f, 30}; // Almost no diffuse, very reflective
        case MaterialType::Mirror:   return {1.0f, 0.0f, 0.0f, 2};  // Perfect specular reflection, no ambient or diffuse
        case MaterialType::Light:   return  {0.0f, 1.0f, 0.0f, 1};  // Lighr source all is ambient
        default: return {0.0f, 0.0f, 0.0f, 0};
    }
}

int Solid::getColorFromMaterial(const float color) {

    float kaR = std::fmod(color, 1.0f);
    kaR = kaR < 0 ? 1.0f + kaR : kaR;
    return (static_cast<int>(kaR * 255));
}

Texture Solid::LoadTextureFromImg(const char* filename)
{
    int width, height, channels;
    // Load image with 4 channels (RGBA) regardless of source format
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);

    if (!data) {
        std::cout << "Failed to load image: " << filename << " - " << stbi_failure_reason() << std::endl;
        return {0, 0, {}};
    }

    // Copy data into vector
    std::vector<unsigned char> image(data, data + (width * height * 4));
    stbi_image_free(data);

    return {width, height, image};
}

void Solid::incAngles(float xAngle, float yAngle, float zAngle) {
    TransformSystem::incAngles(*transform, xAngle, yAngle, zAngle);
}

void Solid::buildOrbitBasis(const slib::vec3& n) {
    TransformSystem::buildOrbitBasis(*transform, n);
}

void Solid::enableCircularOrbit(const slib::vec3& center,
    float radius,
    const slib::vec3& planeNormal,
    float angularSpeedRadiansPerSec,
    float initialPhaseRadians,
    bool /*faceCenter*/)
{
    TransformSystem::enableCircularOrbit(*transform, center, radius,
        planeNormal, angularSpeedRadiansPerSec, initialPhaseRadians);
}

void Solid::disableCircularOrbit() {
    TransformSystem::disableCircularOrbit(*transform);
}

void Solid::updateOrbit(float dt) {
    TransformSystem::updateOrbit(*transform, dt);
}

// Set emissive color for all materials
void Solid::setEmissiveColor(const slib::vec3& color) {
    if (!mesh) return;
    for (auto& kv : mesh->materials) {
        kv.second.Ke = color * 255.0f;
    }
}