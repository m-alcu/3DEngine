#pragma once
#include "Entity.hpp"
#include "ComponentStore.hpp"
#include "TransformComponent.hpp"
#include "LightComponent.hpp"
#include "MeshComponent.hpp"
#include "RotationComponent.hpp"

class Registry {
    EntityGenerator generator_;
    ComponentStore<TransformComponent> transforms_;
    ComponentStore<LightComponent> lights_;
    ComponentStore<MeshComponent> meshes_;
    ComponentStore<RotationComponent> rotations_;

public:
    Entity createEntity() { return generator_.create(); }

    void destroyEntity(Entity e) {
        transforms_.remove(e);
        lights_.remove(e);
        meshes_.remove(e);
        rotations_.remove(e);
    }

    void clear() {
        transforms_.clear();
        lights_.clear();
        meshes_.clear();
        rotations_.clear();
    }

    ComponentStore<TransformComponent>& transforms() { return transforms_; }
    const ComponentStore<TransformComponent>& transforms() const { return transforms_; }

    ComponentStore<LightComponent>& lights() { return lights_; }
    const ComponentStore<LightComponent>& lights() const { return lights_; }

    ComponentStore<MeshComponent>& meshes() { return meshes_; }
    const ComponentStore<MeshComponent>& meshes() const { return meshes_; }

    ComponentStore<RotationComponent>& rotations() { return rotations_; }
    const ComponentStore<RotationComponent>& rotations() const { return rotations_; }
};
