#pragma once
#include "solid.hpp"

class Icosahedron : public Solid {
public:
    Icosahedron() {}

protected:
    void loadVertices() override;
    void loadFaces() override;
};