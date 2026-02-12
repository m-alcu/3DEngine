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
    TransformSystem::updateTransform(transform);
}

void Solid::calculateFaceNormals() {

    for (int i = 0; i < numFaces; i++) {
        const Face &face = Solid::faceData[i].face;
        const size_t n = face.vertexIndices.size();

        // Newell's method: works for any polygon (tri, quad, n-gon)
        // and handles degenerate edges gracefully
        // https://every-algorithm.github.io/2024/03/06/newells_algorithm.html
        slib::vec3 normal = {0.0f, 0.0f, 0.0f};

        for (size_t j = 0; j < n; ++j) {
            const slib::vec3& curr = Solid::vertexData[face.vertexIndices[j]].vertex;
            const slib::vec3& next = Solid::vertexData[face.vertexIndices[(j + 1) % n]].vertex;

            normal.x += (curr.y - next.y) * (curr.z + next.z);
            normal.y += (curr.z - next.z) * (curr.x + next.x);
            normal.z += (curr.x - next.x) * (curr.y + next.y);
        }
        Solid::faceData[i].faceNormal = smath::normalize(normal);
    }
}

void Solid::calculateVertexNormals() {

    for (int i = 0; i < numVertices; i++) {
        slib::vec3 vertexNormal = { 0, 0, 0 };
        for(int j = 0; j < numFaces; j++) {

            for (int vi : Solid::faceData[j].face.vertexIndices) {
                // guard in case your data can contain bad indices
                if (vi == i) {
                    vertexNormal += Solid::faceData[j].faceNormal;
                }
            }        
        }
        Solid::vertexData[i].normal = smath::normalize(vertexNormal);
    }

}

void Solid::calculateMinMaxCoords() {
    if (numVertices == 0) {
        minCoord = {0.0f, 0.0f, 0.0f};
        maxCoord = {0.0f, 0.0f, 0.0f};
        return;
    }

    // Initialize with the first vertex
    minCoord = vertexData[0].vertex;
    maxCoord = vertexData[0].vertex;

    // Iterate through all vertices to find min and max coordinates
    for (int i = 1; i < numVertices; i++) {
        const slib::vec3& v = vertexData[i].vertex;

        // Update minimum coordinates
        if (v.x < minCoord.x) minCoord.x = v.x;
        if (v.y < minCoord.y) minCoord.y = v.y;
        if (v.z < minCoord.z) minCoord.z = v.z;

        // Update maximum coordinates
        if (v.x > maxCoord.x) maxCoord.x = v.x;
        if (v.y > maxCoord.y) maxCoord.y = v.y;
        if (v.z > maxCoord.z) maxCoord.z = v.z;
    }
}

float Solid::getBoundingRadius() const {
    slib::vec3 center{(minCoord.x + maxCoord.x) * 0.5f,
                      (minCoord.y + maxCoord.y) * 0.5f,
                      (minCoord.z + maxCoord.z) * 0.5f};
    slib::vec3 halfDiag{maxCoord.x - center.x, maxCoord.y - center.y,
                        maxCoord.z - center.z};
    return smath::distance(halfDiag);
}

slib::vec3 Solid::getWorldCenter() const {
    return TransformSystem::getWorldCenter(transform, minCoord, maxCoord);
}

void Solid::updateWorldBounds(slib::vec3& worldBoundMin, slib::vec3& worldBoundMax) const {
    TransformSystem::updateWorldBounds(transform, minCoord, maxCoord, worldBoundMin, worldBoundMax);
}

void Solid::scaleToRadius(float targetRadius) {
    TransformSystem::scaleToRadius(transform, getBoundingRadius(), targetRadius);
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
    TransformSystem::incAngles(transform, xAngle, yAngle, zAngle);
}

void Solid::buildOrbitBasis(const slib::vec3& n) {
    TransformSystem::buildOrbitBasis(transform, n);
}

void Solid::enableCircularOrbit(const slib::vec3& center,
    float radius,
    const slib::vec3& planeNormal,
    float angularSpeedRadiansPerSec,
    float initialPhaseRadians,
    bool /*faceCenter*/)
{
    TransformSystem::enableCircularOrbit(transform, center, radius,
        planeNormal, angularSpeedRadiansPerSec, initialPhaseRadians);
}

void Solid::disableCircularOrbit() {
    TransformSystem::disableCircularOrbit(transform);
}

void Solid::updateOrbit(float dt) {
    TransformSystem::updateOrbit(transform, dt);
}

// Set emissive color for all materials
void Solid::setEmissiveColor(const slib::vec3& color) {
    for (auto& kv : materials) {
        kv.second.Ke = color * 255.0f;
    }
}