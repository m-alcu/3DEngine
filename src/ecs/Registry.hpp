#pragma once
#include "Entity.hpp"
#include "ComponentStore.hpp"
#include "TransformComponent.hpp"
#include "LightComponent.hpp"

class Registry {
    EntityGenerator generator_;
    ComponentStore<TransformComponent> transforms_;
    ComponentStore<LightComponent> lights_;

public:
    Entity createEntity() { return generator_.create(); }

    void destroyEntity(Entity e) {
        transforms_.remove(e);
        lights_.remove(e);
    }

    void clear() {
        transforms_.clear();
        lights_.clear();
    }

    ComponentStore<TransformComponent>& transforms() { return transforms_; }
    const ComponentStore<TransformComponent>& transforms() const { return transforms_; }

    ComponentStore<LightComponent>& lights() { return lights_; }
    const ComponentStore<LightComponent>& lights() const { return lights_; }
};
