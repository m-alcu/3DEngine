#pragma once

enum class Shading {
    Wireframe,
    Flat,
    Gouraud,
    BlinnPhong,
    Phong,
    TexturedFlat,
    TexturedGouraud,
    TexturedBlinnPhong,
    TexturedPhong,
    EnvironmentMap
};

// Labels for the enum (must match order of enum values)
static const char* shadingNames[] = {
    "Wireframe",
    "Flat",
    "Gouraud",
    "Blinn-Phong",
    "Phong",
    "Textured Flat",
    "Textured Gouraud",
    "Textured Blinn-Phong",
    "Textured Phong",
    "Environment Map"
};
