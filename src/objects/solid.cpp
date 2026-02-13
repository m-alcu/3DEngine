#include <iostream>
#include <math.h>
#include "solid.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/TransformSystem.hpp"
#include "../ecs/MeshSystem.hpp"
#include "../vendor/stb/stb_image.h"

Solid::Solid() {
}

void Solid::calculateTransformMat() {
    TransformSystem::updateTransform(*transform);
}

void Solid::calculateFaceNormals() {
    if (!mesh) return;
    MeshSystem::updateFaceNormals(*mesh);
}

void Solid::calculateVertexNormals() {
    if (!mesh) return;
    MeshSystem::updateVertexNormals(*mesh);
}

void Solid::calculateMinMaxCoords() {
    if (!mesh) return;
    MeshSystem::updateMinMaxCoords(*mesh);
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
