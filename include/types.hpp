// Basic types used across the simulation
#pragma once

#include <cstddef>
#include <vector>

namespace snow
{

    struct Params
    {
        float wind_speed;         // in m/s
        float total_sim_time;     // in hours
        float time_step_duration; // in sec

        int total_time_steps; // number of steps

        int steps_per_frame;
        // ... add more as needed
    };

    struct Grid
    {
        float Lx; // physical width in meters (x direction)
        float Ly; // physical height in meters (y direction)
        float dx; // cell size in x (meters)
        float dy; // cell size in y (meters)

        std::size_t nx; // number of cells in x (computed)
        std::size_t ny; // number of cells in y (computed)

        std::vector<float> data; // CPU version
        // Later: GPU device pointer for CUDA
    };

} // namespace snow
