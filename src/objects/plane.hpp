#pragma once

#include "solid.hpp"

class Plane : public Solid {
public:
    float size;

    Plane(float planeSize = 10.f) : size(planeSize) {
    }

protected:
    void loadVertices() override;
    void loadFaces() override;
};
