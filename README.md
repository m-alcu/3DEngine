# 3D Rendering Engine

Software 3D engine using SDL3, Dear ImGui, and modern CMake.

## Dependencias
- **SDL3** (biblioteca y headers disponibles para el compilador).
- **CMake** ≥ 3.28.
- Toolchain C++20 con soporte para las extensiones usadas por SDL (GCC/Clang/MSVC o `emcc`).
- Opcional: Emscripten SDK para builds WebAssembly.

## Configuración y compilación (desktop)
1. Generar archivos de construcción:
   ```bash
   cmake -S . -B build
   ```
2. Compilar el binario:
   ```bash
   cmake --build build
   ```
3. Ejecutar el motor:
   ```bash
   ./build/bin/3DEngine
   ```

Ejecuta el binario desde la raíz del repositorio para que los paths relativos de `resources/` funcionen correctamente.

## Tests

El proyecto incluye tests unitarios usando Google Test. Para compilar y ejecutar los tests:

```bash
# Configurar (incluye tests por defecto)
cmake -S . -B build

# Compilar solo los tests
cmake --build build --target test_math --config Release

# Ejecutar tests directamente
./build/bin/Release/test_math.exe   # Windows
./build/bin/test_math               # Linux/macOS

# O usar CTest
cd build && ctest -C Release --output-on-failure
```

Para desactivar la compilación de tests:
```bash
cmake -S . -B build -DBUILD_TESTS=OFF
```

## Escenas disponibles
Seleccionables desde el combo "Scene" en la ventana de ImGui:
- **Torus**, **Tetrakis**, **Icosahedron**, **Cube**, **Knot**, **Star**: primitivas/solids paramétricos con rotación automática opcional.
- **Amiga**: escena inspirada en el clásico logo/banda Amiga.
- **World**: escena que carga geometría desde recursos externos.

## Controles principales
- **Movimiento estilo Descent**: Flechas o keypad para pitch/yaw, `Q`/`E` (o keypad 7/9) para roll, `A`/`Z` (o keypad ±) para avanzar/retroceder.
- **Órbita con el ratón**: mantener clic derecho y arrastrar para orbitar; rueda del ratón para acercar/alejar. Se desactiva el modo vuelo libre mientras se orbita.
- **ImGui**: ajustar velocidad y sensibilidad de cámara, sombreado de los sólidos, tipo de fondo y la escena activa.
- **Escape**: salir.

## Problemas comunes
- **Assets no encontrados**: asegúrate de ejecutar el binario desde la raíz del proyecto para que los paths relativos apunten a `resources/`. Si lo lanzas desde otro directorio, usa `--workdir` o ajusta las rutas de recursos en el código.
- **SDL3 no detectado**: verifica que los headers y la librería estén en las rutas de tu toolchain (`CMAKE_PREFIX_PATH`, `SDL3_DIR`, o variables de entorno como `PKG_CONFIG_PATH`).
- **Compilador antiguo**: se requiere un compilador con soporte C++20; actualiza GCC/Clang o usa el toolchain provisto por tu SDK.

## Build para WebAssembly (Emscripten)
1. Activa el entorno de Emscripten (`source /path/to/emsdk_env.sh`).
2. Genera con la toolchain de Emscripten (puedes usar `emcmake` para simplificar las rutas):
   ```bash
   emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
   ```
3. Compila:
   ```bash
   cmake --build build-wasm
   ```
4. Sirve los artefactos generados (HTML/JS/WASM) con un servidor web y abre el HTML principal en el navegador. Asegúrate de habilitar el preloading de recursos en el script de despliegue si necesitas texturas o escenas externas.

