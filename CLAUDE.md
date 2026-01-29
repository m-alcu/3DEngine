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

## Testing

Unit tests use Google Test (fetched automatically via CMake FetchContent).

```bash
# Build tests (included by default)
cmake --build build --target test_math --config Release

# Run tests directly
./build/bin/Release/test_math.exe   # Windows
./build/bin/test_math               # Linux/macOS

# Run via CTest
cd build && ctest -C Release --output-on-failure

# Disable tests during configuration
cmake -S . -B build -DBUILD_TESTS=OFF
```

Test files are in `tests/`. Current test coverage:
- `test_math.cpp` - Tests for math library (slib vectors/matrices, smath transforms)

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

Each shading mode is implemented as an Effect class with three pipeline stages:

1. **VertexShader** - Transforms object vertices to world space, computes NDC coordinates, calculates rotated normals, and projects to screen coordinates.

2. **GeometryShader** - Recalculates broken vertices that result from clipping (polygons can gain vertices when clipped against the view frustum).

3. **PixelShader** - Per-pixel calculations specific to the shading mode (lighting, shadows, texture sampling).

### Lighting System

The engine supports multiple dynamic light sources. Any `Solid` can act as a light source by setting `lightSourceEnabled = true`.

**Light properties** (`src/light.hpp`):
- `LightType` - Directional, Point, or Spot
- `color` - RGB light color
- `intensity` - Light brightness multiplier
- `position` - World position (for Point/Spot lights)
- `direction` - Light direction (for Directional/Spot lights)
- Attenuation parameters for distance falloff

**Multi-light rendering**:
- Scene provides `lightSources()` method returning a C++20 filtered view of all light-source solids
- PixelShaders iterate over all light sources and accumulate color contributions
- Each light's contribution includes diffuse, specular (for Phong/BlinnPhong), attenuation, and shadow factors

### Shadow Mapping

Each light-source solid maintains its own shadow map for shadow calculations.

**Key components**:
- `ShadowMap` (`src/ShadowMap.hpp`) - Depth buffer rendered from light's perspective
- `Solid::shadowMap` - Each light-source solid owns a `std::shared_ptr<ShadowMap>`
- `Renderer::renderShadowPass()` - Renders depth from each light source before main rendering

**Shadow rendering flow**:
1. For each solid with `lightSourceEnabled`, build light matrices and render scene depth to its shadow map
2. During main rendering, PixelShaders sample shadow maps to determine shadow factor per light
3. PCF (Percentage Closer Filtering) support with configurable radius for soft shadow edges

**ImGui controls**:
- Toggle shadows on/off
- PCF radius selection (Off, 3x3, 5x5)
- Shadow map debug overlay display
- Per-solid light intensity slider (when solid is a light source)

### Camera Controls

Descent-style 6DOF movement with momentum/hysteresis. Right-click drag for orbit mode around the target solid.

## Dependencies

- SDL3 (as git submodule in `submodules/SDL/`)
- Dear ImGui (vendored in `src/vendor/imgui/`)
- LodePNG (vendored in `src/vendor/lodepng/`)
- Google Test (fetched via CMake FetchContent for testing)
- C++23 standard required
