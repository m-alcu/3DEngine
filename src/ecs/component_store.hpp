#pragma once
#include "entity.hpp"
#include <unordered_map>

template<typename T>
class ComponentStore {
    std::unordered_map<Entity, T> data_;

public:
    void add(Entity e, T component) { data_.emplace(e, std::move(component)); }

    void remove(Entity e) { data_.erase(e); }

    T* get(Entity e) {
        auto it = data_.find(e);
        return it != data_.end() ? &it->second : nullptr;
    }

    const T* get(Entity e) const {
        auto it = data_.find(e);
        return it != data_.end() ? &it->second : nullptr;
    }

    bool has(Entity e) const { return data_.count(e) > 0; }

    size_t size() const { return data_.size(); }

    void clear() { data_.clear(); }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
};
