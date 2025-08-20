#include <iostream>
#include <math.h>
#include "../rasterizer.hpp"
#include "../smath.hpp"
#include "../vendor/lodepng/lodepng.h"

void Solid::calculateNormals() {

    for (int i = 0; i < numFaces; i++) {
        const Face &face = Solid::faceData[i].face;
        slib::vec3 v1 = Solid::vertexData[face.vertexIndices[0]].vertex;
        slib::vec3 v2 = Solid::vertexData[face.vertexIndices[1]].vertex;
        slib::vec3 v3 = Solid::vertexData[face.vertexIndices[2]].vertex;

        // Calculate the edge vectors.
        slib::vec3 v21 = v2 - v1;
        slib::vec3 v32 = v3 - v2;

        Solid::faceData[i].faceNormal = smath::normalize(smath::cross(v21, v32));
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
            }        }
        Solid::vertexData[i].normal = smath::normalize(vertexNormal);
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
        default: return {0.0f, 0.0f, 0.0f, 0};
    }
}

int Solid::getColorFromMaterial(const float color) {

    float kaR = std::fmod(color, 1.0f);
    kaR = kaR < 0 ? 1.0f + kaR : kaR;
    return (static_cast<int>(kaR * 255));
}

slib::texture Solid::DecodePng(const char* filename)
{
    std::vector<unsigned char> buffer;
    std::vector<unsigned char> image; // the raw pixels
    lodepng::load_file(buffer, filename);
    unsigned width, height;

    lodepng::State state;

    // decode
    unsigned error = lodepng::decode(image, width, height, state, buffer);
    const LodePNGColorMode& color = state.info_png.color;
    // Use the RAW (decoded) format to compute bytes per pixel
    unsigned bits_per_pixel = lodepng_get_bpp(&state.info_raw); // e.g. 24 for RGB8, 32 for RGBA8
    unsigned bpp_bytes = bits_per_pixel / 8;               // 3 or 4
    unsigned rowStride = bpp_bytes * width;
    // if there's an error, display it
    if (error)
    {
        std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        exit(1);
    }

    // the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw
    // it, ...
    return {static_cast<int>(width), static_cast<int>(height), image, bpp_bytes, rowStride};
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


