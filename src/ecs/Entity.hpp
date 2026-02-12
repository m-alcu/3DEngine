#pragma once
#include <cstdint>

using Entity = uint32_t;
constexpr Entity NULL_ENTITY = 0;

class EntityGenerator {
    Entity next_ = 1;
public:
    Entity create() { return next_++; }
};
