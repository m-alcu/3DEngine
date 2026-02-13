#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include "../slib.hpp"
#include "../constants.hpp"
#include "../smath.hpp"
#include "../ecs/TransformComponent.hpp"
#include "../ecs/Entity.hpp"
#include "../ecs/LightComponent.hpp"
#include "../ecs/MeshComponent.hpp"
#include "../ecs/MaterialComponent.hpp"
#include "../ecs/ShadowComponent.hpp"
#include "../ecs/RotationComponent.hpp"
#include "../ecs/RenderComponent.hpp"


class Solid {
public:
    Entity entity = NULL_ENTITY;
    RenderComponent localRender_;
    RenderComponent* render = &localRender_;
    TransformComponent localTransform_;
    TransformComponent* transform = &localTransform_;

    MeshComponent localMesh_;
    MeshComponent* mesh = &localMesh_;
    MaterialComponent localMaterial_;
    MaterialComponent* materialComponent = &localMaterial_;
    RotationComponent localRotation_;
    RotationComponent* rotation = &localRotation_;
    std::string name;
    std::optional<LightComponent> localLight_;
    LightComponent* lightComponent = nullptr;
    std::optional<ShadowComponent> localShadow_;
    ShadowComponent* shadowComponent = nullptr;
 
public:
    // Base constructor that initializes common data members.
    Solid();

    // Virtual destructor for proper cleanup in derived classes.
    virtual ~Solid() = default;

    // A common setup method that calls the helper functions.
    virtual void setup();

    virtual void calculateFaceNormals();

    virtual void calculateVertexNormals();

    virtual void calculateMinMaxCoords();

    slib::vec3 getWorldCenter() const;

    void updateWorldBounds(slib::vec3& minV, slib::vec3& maxV) const;

    void scaleToRadius(float targetRadius);

    void calculateTransformMat();

    virtual void incAngles(float xAngle, float yAngle, float zAngle);

    virtual void buildOrbitBasis(const slib::vec3& n);

    // Enable a circular orbit
    virtual void enableCircularOrbit(const slib::vec3& center,
        float radius,
        const slib::vec3& planeNormal,
        float angularSpeedRadiansPerSec,
        float initialPhaseRadians = 0.0f,
        bool faceCenter = false);

    virtual void disableCircularOrbit();

    virtual void updateOrbit(float dt);

    void initLight(LightComponent lc) {
        localLight_ = std::move(lc);
        lightComponent = &*localLight_;
        localShadow_ = ShadowComponent{};
        shadowComponent = &*localShadow_;
    }

protected:
    // Protected virtual methods to be implemented by derived classes.
    virtual void loadVertices() = 0;
    virtual void loadFaces() = 0;
	
};
