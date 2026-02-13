#pragma once
#include "Entity.hpp"
#include "ComponentStore.hpp"
#include "TransformComponent.hpp"
#include "LightComponent.hpp"
#include "MeshComponent.hpp"
#include "MaterialComponent.hpp"
#include "ShadowComponent.hpp"
#include "NameComponent.hpp"
#include "RotationComponent.hpp"
#include "RenderComponent.hpp"

class Registry {
    EntityGenerator generator_;
    ComponentStore<TransformComponent> transforms_;
    ComponentStore<LightComponent> lights_;
    ComponentStore<MeshComponent> meshes_;
    ComponentStore<MaterialComponent> materials_;
    ComponentStore<ShadowComponent> shadows_;
    ComponentStore<NameComponent> names_;
    ComponentStore<RotationComponent> rotations_;
    ComponentStore<RenderComponent> renders_;

public:
    Entity createEntity() { return generator_.create(); }

    void destroyEntity(Entity e) {
        transforms_.remove(e);
        lights_.remove(e);
        meshes_.remove(e);
        materials_.remove(e);
        shadows_.remove(e);
        names_.remove(e);
        rotations_.remove(e);
        renders_.remove(e);
    }

    void clear() {
        transforms_.clear();
        lights_.clear();
        meshes_.clear();
        materials_.clear();
        shadows_.clear();
        names_.clear();
        rotations_.clear();
        renders_.clear();
    }

    ComponentStore<TransformComponent>& transforms() { return transforms_; }
    const ComponentStore<TransformComponent>& transforms() const { return transforms_; }

    ComponentStore<LightComponent>& lights() { return lights_; }
    const ComponentStore<LightComponent>& lights() const { return lights_; }

    ComponentStore<MeshComponent>& meshes() { return meshes_; }
    const ComponentStore<MeshComponent>& meshes() const { return meshes_; }

    ComponentStore<MaterialComponent>& materials() { return materials_; }
    const ComponentStore<MaterialComponent>& materials() const { return materials_; }

    ComponentStore<ShadowComponent>& shadows() { return shadows_; }
    const ComponentStore<ShadowComponent>& shadows() const { return shadows_; }

    ComponentStore<NameComponent>& names() { return names_; }
    const ComponentStore<NameComponent>& names() const { return names_; }

    ComponentStore<RotationComponent>& rotations() { return rotations_; }
    const ComponentStore<RotationComponent>& rotations() const { return rotations_; }

    ComponentStore<RenderComponent>& renders() { return renders_; }
    const ComponentStore<RenderComponent>& renders() const { return renders_; }
};
