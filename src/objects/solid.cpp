#include <iostream>
#include <math.h>
#include "solid.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include "../vendor/lodepng/lodepng.h"

Solid::Solid()
    : modelMatrix(smath::identity()),
      normalMatrix(smath::identity()) {
}

void Solid::calculateTransformMat() {
    slib::mat4 rotate = smath::rotation(
        slib::vec3{position.xAngle, position.yAngle, position.zAngle});
    slib::mat4 translate = smath::translation(
        slib::vec3{position.x, position.y, position.z});
    slib::mat4 scale = smath::scale(
        slib::vec3{position.zoom, position.zoom, position.zoom});
    modelMatrix = translate * rotate * scale;
    normalMatrix = rotate;
}

void Solid::calculateNormals() {

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
    slib::vec3 localCenter{(minCoord.x + maxCoord.x) * 0.5f,
                           (minCoord.y + maxCoord.y) * 0.5f,
                           (minCoord.z + maxCoord.z) * 0.5f};
    slib::vec4 world = modelMatrix * slib::vec4(localCenter, 1.0f);
    return {world.x, world.y, world.z};
}

void Solid::updateWorldBounds(slib::vec3& worldBoundMin, slib::vec3& worldBoundMax) const {
    // 8 corners of the AABB
    slib::vec3 corners[8] = {
        {minCoord.x, minCoord.y, minCoord.z}, {minCoord.x, minCoord.y, maxCoord.z},
        {minCoord.x, maxCoord.y, minCoord.z}, {minCoord.x, maxCoord.y, maxCoord.z},
        {maxCoord.x, minCoord.y, minCoord.z}, {maxCoord.x, minCoord.y, maxCoord.z},
        {maxCoord.x, maxCoord.y, minCoord.z}, {maxCoord.x, maxCoord.y, maxCoord.z}};

    for (const auto& corner : corners) {
        slib::vec4 world = modelMatrix * slib::vec4(corner, 1.0f);
        worldBoundMin.x = std::min(worldBoundMin.x, world.x);
        worldBoundMin.y = std::min(worldBoundMin.y, world.y);
        worldBoundMin.z = std::min(worldBoundMin.z, world.z);
        worldBoundMax.x = std::max(worldBoundMax.x, world.x);
        worldBoundMax.y = std::max(worldBoundMax.y, world.y);
        worldBoundMax.z = std::max(worldBoundMax.z, world.z);
    }
}

void Solid::scaleToRadius(float targetRadius) {
    float radius = getBoundingRadius();
    if (radius > 0.0f) {
        position.zoom *= targetRadius / radius;
    }
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

Texture Solid::DecodePng(const char* filename)
{
    std::vector<unsigned char> buffer;
    std::vector<unsigned char> image; // the raw pixels
    lodepng::load_file(buffer, filename);
    unsigned width, height;

    lodepng::State state;

    // decode
    unsigned error = lodepng::decode(image, width, height, state, buffer);
    // if there's an error, display it
    if (error)
    {
        std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        exit(1);
    }

    // the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw
    // it, ...
    return {static_cast<int>(width), static_cast<int>(height), image};
}

void Solid::rotate(float xAngle, float yAngle, float zAngle) {
	position.xAngle += xAngle;
	position.yAngle += yAngle;
	position.zAngle += zAngle;
}

static inline float wrapTwoPi(float a) {
    const float TWO_PI = 6.2831853071795864769f;
    a = std::fmod(a, TWO_PI);
    if (a < 0) a += TWO_PI;
    return a;
}

// Build U,V in the plane orthogonal to n (n must be unit)
void Solid::buildOrbitBasis(const slib::vec3& n) {
    // pick a vector not parallel to n
    slib::vec3 a = (std::fabs(n.x) < 0.9f) ? slib::vec3{ 1,0,0 } : slib::vec3{ 0,1,0 };
    orbitU = smath::normalize(smath::cross(n, a));
    orbitV = smath::normalize(smath::cross(n, orbitU));
}

// Enable a circular orbit
void Solid::enableCircularOrbit(const slib::vec3& center,
    float radius,
    const slib::vec3& planeNormal,
    float angularSpeedRadiansPerSec,
    float initialPhaseRadians,
    bool faceCenter)
{
    orbit_.center = center;
    orbit_.radius = radius;
    orbit_.n = smath::normalize(planeNormal);
    orbit_.omega = angularSpeedRadiansPerSec;
    orbit_.phase = initialPhaseRadians;
    orbit_.enabled = true;
    buildOrbitBasis(orbit_.n);
}

// Disable the orbit motion
void Solid::disableCircularOrbit() { orbit_.enabled = false; }

// Call once per frame with delta time (seconds)
void Solid::updateOrbit(float dt)
{
    if (!orbit_.enabled) return;

    orbit_.phase = wrapTwoPi(orbit_.phase + orbit_.omega * dt);

    float c = std::cos(orbit_.phase);
    float s = std::sin(orbit_.phase);

    // Position on the circle in the U-V plane
    slib::vec3 P = orbit_.center + (orbitU * c + orbitV * s) * orbit_.radius;

    position.x = P.x;
    position.y = P.y;
    position.z = P.z;

}

// Set emissive color for all materials
void Solid::setEmissiveColor(const slib::vec3& color) {
    for (auto& kv : materials) {
        kv.second.Ke = color * 255.0f;
    }
}