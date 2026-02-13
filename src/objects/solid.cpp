#include <iostream>
#include <math.h>
#include "solid.hpp"
#include "../slib.hpp"
#include "../smath.hpp"
#include "../ecs/TransformSystem.hpp"
#include "../ecs/MeshSystem.hpp"

Solid::Solid() {
}

void Solid::setup() {
    loadVertices();
    loadFaces();
    calculateFaceNormals();
    calculateVertexNormals();
    calculateMinMaxCoords();
    if (mesh) {
        MeshSystem::markBoundsDirty(*mesh);
    }
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

slib::vec3 Solid::getWorldCenter() const {
    if (!mesh) return {0.0f, 0.0f, 0.0f};
    return TransformSystem::getWorldCenter(*transform, mesh->minCoord, mesh->maxCoord);
}

void Solid::updateWorldBounds(slib::vec3& worldBoundMin, slib::vec3& worldBoundMax) const {
    if (!mesh) return;
    TransformSystem::updateWorldBounds(*transform, mesh->minCoord, mesh->maxCoord, worldBoundMin, worldBoundMax);
}

void Solid::scaleToRadius(float targetRadius) {
    if (!mesh) return;
    TransformSystem::scaleToRadius(*transform, MeshSystem::getBoundingRadius(*mesh), targetRadius);
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
