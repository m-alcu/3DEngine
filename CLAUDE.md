# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Configure (desktop)
cmake -S . -B build

# Build
cmake --build build

# Run (from repo root for resources/ path resolution)
./build/bin/3DEngine

# WebAssembly build (requires Emscripten SDK)
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
```

The binary must be run from the repository root so relative paths to `resources/` work correctly.

## Architecture Overview

This is a software 3D rendering engine (no GPU/OpenGL) using SDL3 for windowing and pixel buffer display, Dear ImGui for the UI, and a custom software rasterizer.

### Core Rendering Pipeline

1. **Scene** (`src/scene.hpp`) - Container holding solids, camera, lights, pixel buffer, z-buffer, and projection/view matrices. Derived scene classes (in `src/scenes/`) configure specific demo scenes.

2. **Solid** (`src/objects/solid.hpp`) - Abstract base class for 3D objects. Stores vertex data, face data, normals, materials. Derived classes (Torus, Cube, Icosahedron, etc.) implement `loadVertices()` and `loadFaces()`.

3. **Renderer** (`src/renderer.hpp`) - Dispatches rendering based on shading mode. Owns multiple `Rasterizer<Effect>` instances.

4. **Rasterizer** (`src/rasterizer.hpp`) - Template class parameterized by Effect type. Handles:
   - Transform matrix calculation (model -> world)
   - Vertex processing via Effect's vertex shader
   - Face visibility (backface culling)
   - Polygon clipping (Sutherland-Hodgman)
   - Scanline rasterization with z-buffer

5. **Effects** (`src/effects/`) - Each effect defines a `Vertex` struct plus `VertexShader`, `GeometryShader`, `PixelShader` classes:
   - `FlatEffect` - Per-face flat shading
   - `GouraudEffect` - Per-vertex lighting interpolated
   - `PhongEffect` / `BlinnPhongEffect` - Per-pixel lighting
   - `Textured*Effect` variants - Add texture sampling

### Math Library

- `slib.hpp` / `slib.cpp` - Custom vector (vec2, vec3, vec4) and matrix (mat4) types with operator overloads
- `smath.hpp` / `smath.cpp` - Transform functions: `perspective()`, `lookAt()`, `fpsview()`, `rotation()`, `scale()`, `translation()`, texture sampling

### Key Patterns

- **Factory pattern**: `SceneFactory` and `BackgroundFactory` create scene/background instances by enum type
- **Effect-based rasterization**: Shading is selected at runtime by switching which `Rasterizer<Effect>` draws each solid
- **Scanline rasterization**: Polygons are drawn using edge-walking slopes (`src/slope.hpp`) and per-pixel z-buffer tests

### Shading Modes

Selectable via ImGui combo: Wireframe, Flat, Gouraud, Phong, Blinn-Phong, plus textured variants of each.

### Camera Controls

Descent-style 6DOF movement with momentum/hysteresis. Right-click drag for orbit mode around the target solid.

## Dependencies

- SDL3 (as git submodule in `submodules/SDL/`)
- Dear ImGui (vendored in `src/vendor/imgui/`)
- LodePNG (vendored in `src/vendor/lodepng/`)
- C++23 standard required
