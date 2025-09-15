#include <iostream>
#include <cmath>
#include "types.hpp"
#include "simulation.hpp"
#include "cpu_backend.hpp"
// Safe to include: provides CPU fallback when SNOWSIM_HAS_CUDA == 0
// and CUDA interface when enabled.
#include "cuda_backend.hpp"

int main()
{
    using namespace snow;

    Params params{};
    params.wind_speed = 5.0f;
    params.total_sim_time = 0.1f;
    params.time_step_duration = 0.1f;
    params.steps_per_frame = 1;

    params.total_time_steps = static_cast<int>(std::lround(params.total_sim_time * 3600.0f / params.time_step_duration));

    // Define physical domain and discretization
    Grid grid{};
    grid.Lx = 1000.0f; // meters
    grid.Ly = 200.0f;  // meters
    grid.dx = 10.0f;   // meters
    grid.dy = 10.0f;   // meters

    grid.nx = static_cast<std::size_t>(std::lround(grid.Lx / grid.dx));
    grid.ny = static_cast<std::size_t>(std::lround(grid.Ly / grid.dy));

    grid.data.assign(grid.nx * grid.ny, 0.0f);

// compile with both backends then choose which one to use at run time
#if SNOWSIM_HAS_CUDA
    cuda::CUDASimulation sim; // Uses real CUDA backend
#else
    cpu::CPUSimulation sim; // Fallback when CUDA is unavailable
#endif

    for (int t = 0; t < params.total_time_steps; ++t)
    {
        sim.step(grid, params);
    }

    std::cout << "Finished simulation steps: grid(" << grid.nx << "x" << grid.ny << ")\n";
    return 0;
}
