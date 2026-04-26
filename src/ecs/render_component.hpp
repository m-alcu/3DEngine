#pragma once

enum class Shading {
    Wireframe,
    Flat,
    Gouraud,
    Phong,
    TexturedFlat,
    TexturedGouraud,
    TexturedPhong,
    EnvironmentMap
};

// Labels for the enum (must match order of enum values)
static const char* shadingNames[] = {
    "Wireframe",
    "Flat",
    "Gouraud",
    "Phong",
    "Textured Flat",
    "Textured Gouraud",
    "Textured Phong",
    "Environment Map"
};

struct RenderComponent {
    Shading shading = Shading::Flat;
};
