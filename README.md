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

## Code Layout

- `include/`: public headers (`types.hpp`, `simulation.hpp`, backends)
- `src/`: core sources (`main.cpp`, CPU/CUDA backends)
- `viz/`: renderer and UI scaffolding (not yet wired into the build)

## Notes

- Params fields use consistent names and units: `time_step_duration` (seconds), `total_time_steps` is an integer.
- Grid sizes (`nx`, `ny`) are `size_t` to match container sizes.
- The app rounds step count and grid resolution using `std::lround` for predictable discretization.

