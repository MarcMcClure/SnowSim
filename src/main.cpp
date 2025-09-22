#include <algorithm>
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

    //setting up params
    #pragma region

    Params params{};
    params.wind_speed = 4.9f;               // m/sec
    params.settling_speed = 0.06f;          // m/sec
    params.precipitation_rate = 0.1f;       // g/m^2/sec
    params.settaled_snow_density = 200000;  // g/m^2
    params.ground_height = 25.0f;           // m above y=0

    params.Lx = 1000.0f; // meters
    params.Ly = 200.0f;  // meters
    params.dx = 10.0f;   // meters
    params.dy = 10.0f;   // meters

    params.nx = static_cast<std::size_t>(std::lround(params.Lx / params.dx));
    params.ny = static_cast<std::size_t>(std::lround(params.Ly / params.dy));

    params.total_sim_time = 10.0f  * 3600.0f;    //sec
    params.time_step_duration = 0.2f;           //sec
    params.steps_per_frame = 1;

    params.total_time_steps = static_cast<int>(std::lround(params.total_sim_time / params.time_step_duration));
    #pragma endregion

    // Define physical domain and discretization, setting up fields
    #pragma region
    Fields fields;
    //this mask is used for tiles where the groud level is over the center of the tile. ground tiles are 0 air tiles are 1
    fields.air_mask = Field2D<uint8_t>(params.nx, params.ny, 1);
    for (std::size_t i = 0; i < fields.air_mask.nx; i++){
        for (std::size_t j = 0; j < fields.air_mask.ny; j++){
            if((j + 0.5f)*params.dy <= params.ground_height) fields.air_mask(i,j) = 0;
            else continue;
        }
    }
    fields.snow_density = Field2D<float>(params.nx, params.ny);
    fields.next_snow_density = Field2D<float>(params.nx, params.ny);
    fields.snow_transport_speed_x = Field2D<float>(params.nx + 1, params.ny, params.wind_speed);
    fields.snow_transport_speed_y = Field2D<float>(params.nx, params.ny + 1, -params.settling_speed);
    fields.precipitation_source = Field1D<float>(params.nx, params.precipitation_rate);
    // Inflow at x=0: match the supplied precipitation with the local vertical motion,
    // then scale by the upwind horizontal speed to get a lateral source term (g/(m^2*s)).
    fields.windborn_horizontal_source = Field1D<float>(params.ny, 0.0f);
    {
        for (std::size_t j = 0; j < params.ny; ++j)
        {
            const float vy_bottom = fields.snow_transport_speed_y(0, j);
            const float vy_top = fields.snow_transport_speed_y(0, j + 1);
            const float vy_avg = 0.5f * (vy_bottom + vy_top); // boundary vertical velocity (m/s)
            const float abs_vy = std::fabs(vy_avg);
            const float vy_mag = abs_vy > 1e-6f ? abs_vy : 1e-6f; // prevent divide-by-zero
            const float ux = fields.snow_transport_speed_x(0, j); // left boundary face (m/s)
            const float ux_in = ux > 0.0f ? ux : 0.0f; // only inflow contributes
            const float lateral_scale = ux_in / vy_mag; // dimensionless ratio of speeds
            const bool is_air = fields.air_mask(0, j) != 0;
            fields.windborn_horizontal_source(j) = is_air ? params.precipitation_rate * lateral_scale : 0.0f; // g/(m^2*s)
        }
    }
    fields.snow_accumulation_mass = Field1D<float>(params.nx);
    fields.snow_accumulation_density = Field1D<float>(params.nx,params.settaled_snow_density);
    #pragma endregion

// compile with both backends then choose which one to use at run time
#if SNOWSIM_HAS_CUDA
    cuda::CUDASimulation sim; // Uses real CUDA backend
#else
    cpu::CPUSimulation sim; // Fallback when CUDA is unavailable
#endif

    // CFL check: warn if a single step could advect snow beyond immediate neighbours.
    #pragma region
    float max_abs_ux = 0.0f;
    for (const float ux : fields.snow_transport_speed_x.data)
    {
        max_abs_ux = std::max(max_abs_ux, std::fabs(ux));
    }
    float max_abs_vy = 0.0f;
    for (const float vy : fields.snow_transport_speed_y.data)
    {
        max_abs_vy = std::max(max_abs_vy, std::fabs(vy));
    }
    const float cfl_x = max_abs_ux * params.time_step_duration / params.dx;
    const float cfl_y = max_abs_vy * params.time_step_duration / params.dy;
    if (cfl_x > 1.0f || cfl_y > 1.0f)
    {
        std::cerr << "Warning: CFL condition exceeded (CFL_x=" << cfl_x
                  << ", CFL_y=" << cfl_y << ")\n";
    }
    #pragma endregion
    // TODO: Refine CFL safety check to capture local variations and per-direction thresholds.

    // sim loop
    for (int t = 0; t < params.total_time_steps; ++t)
    {
        sim.step(fields, params);
    }

    std::cout << "accumulated snow" << ":\n";
    for (int i = 0; i < params.nx; i++) {
        std::cout << fields.snow_accumulation_mass.data[i] << "\n";
    }
    std::cout << "\n";

    std::cout << "Finished simulation steps: grid(" << params.nx << "x" << params.ny << ")\n";
    return 0;
}

