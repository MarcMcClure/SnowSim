# SnowSim

A simple 2D snow accumulation simulation with a pluggable backend design (CPU by default, optional CUDA).

## Build

Prerequisites: CMake 3.18+, a C++17 compiler. CUDA toolkit optional if enabling the CUDA backend.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### Enable CUDA backend

```
cmake -S . -B build -DENABLE_CUDA=ON
cmake --build build --config Debug
```

If CUDA is unavailable, the CPU backend remains usable.

## Run

From the build directory (Visual Studio generators place binaries under configuration subfolders):

```
./build/Debug/snow_sim_app.exe
```

## Testing

Unit tests are built with Catch2’s amalgamated release (vendored in `tests/unit/`). By default they are included when configuring the project; to override this, toggle the `SNOWSIM_ENABLE_TESTS` option:

```
cmake -S . -B build -DSNOWSIM_ENABLE_TESTS=ON
cmake --build build --config Debug --target snow_sim_unit_tests
ctest --build-config Debug
```

## Code Layout

- `include/`: public headers (`types.hpp`, `simulation.hpp`, backends)
- `src/`: core sources (`main.cpp`, CPU/CUDA backends)
- `viz/`: renderer and UI scaffolding (GLFW/GLAD window bootstrap)

## Visualization Dependencies

The visualization currently relies on GLFW for the window/context and GLAD for OpenGL loading. To refresh or set up a new development machine:

1. Download the 64-bit GLFW binaries (tested with 3.4) https://www.glfw.org/download.html:
   - Copy `include/GLFW` into `external/include/GLFW`.
   - Copy the desired MSVC import library (e.g., `lib-vc2022/glfw3.lib`) into `external/lib/glfw3.lib`.
   - Copy the matching `glfw3.dll` into `external/glfw3.dll` so the post-build step can stage it beside the executable (or copy it manually into the build output directory).
2. Generate a GLAD loader (core profile, same OpenGL version 4.3 `renderer.cpp`) it can be found at https://gen.glad.sh/#generator=c&api=gl%3D4.3&profile=gl%3Dcore%2Cgles1%3Dcommon.
   - Place the generated headers under `external/include/glad`.
   - Add the generated `gl.c` to `viz/` and list it in the `snow_sim_app` sources in `CMakeLists.txt`.
3. Re-run CMake configure so the updated include/lib paths are picked up.

With those assets in place, the default build commands above produce a windowed preview application.
## Notes

- Params fields use consistent names and units: `time_step_duration` (seconds), `total_time_steps` is an integer.
- Grid sizes (`nx`, `ny`) are `size_t` to match container sizes.
- The app rounds step count and grid resolution using `std::lround` for predictable discretization.
- JSON parsing uses the single-header release of [nlohmann/json](https://github.com/nlohmann/json/blob/develop/single_include/nlohmann/json.hpp); the header is vendored under `include/json.hpp`.
- Unit tests use the Catch2 amalgamated build (`tests/unit/catch_amalgamated.hpp/.cpp` vendored from [Catch2](https://github.com/catchorg/Catch2/tree/devel/extras)).


