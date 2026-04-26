#pragma once
#include "entity.hpp"
#include "component_store.hpp"
#include "transform_component.hpp"
#include "light_component.hpp"
#include "mesh_component.hpp"
#include "material_component.hpp"
#include "shadow_component.hpp"
#include "name_component.hpp"
#include "render_component.hpp"

class Registry {
    EntityGenerator generator_;
    ComponentStore<TransformComponent> transforms_;
    ComponentStore<LightComponent> lights_;
    ComponentStore<MeshComponent> meshes_;
    ComponentStore<MaterialComponent> materials_;
    ComponentStore<ShadowComponent> shadows_;
    ComponentStore<NameComponent> names_;
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
        renders_.remove(e);
    }

    void clear() {
        transforms_.clear();
        lights_.clear();
        meshes_.clear();
        materials_.clear();
        shadows_.clear();
        names_.clear();
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

    ComponentStore<RenderComponent>& renders() { return renders_; }
    const ComponentStore<RenderComponent>& renders() const { return renders_; }
};
