#include <algorithm>
#include <iostream>
#include <cmath>
#include <glm/glm/glm.hpp>
#include "types.hpp"
#include "my_helper.hpp"
#include "simulation.hpp"
#include "cpu_backend.hpp"
// Safe to include: provides CPU fallback when SNOWSIM_HAS_CUDA == 0
// and CUDA interface when enabled.
#include "cuda_backend.hpp"
#include "renderer.hpp"

int main()
{
    using namespace snow;

    //setting up params
    #pragma region

    Params params{};
    params.wind_speed = 1.70f;               // m/sec
    params.settling_speed = 0.52f;          // m/sec
    params.precipitation_rate = 0.1f;       // g/m^2/sec
    params.settaled_snow_density = 200000;  // g/m^2
    //TODO: remove ground height
    params.ground_height = 30.0f;           // m above y=0

    params.Lx = 150.0f; // meters
    params.Ly = 150.0f;  // meters
    params.dx = 10.0f;   // meters
    params.dy = 10.0f;   // meters

    params.nx = static_cast<std::size_t>(std::lround(params.Lx / params.dx));
    params.ny = static_cast<std::size_t>(std::lround(params.Ly / params.dy));

    params.total_sim_time = 10.0f  * 3600.0f;    //sec
    params.time_step_duration = 0.01f;           //sec
    params.steps_per_frame = 60;

    params.total_time_steps = static_cast<int>(std::lround(params.total_sim_time / params.time_step_duration));

    params.light_direction = glm::vec3(-0.4f, -1.0f, -0.6f);
    params.light_color = glm::vec3(1.0f, 0.95f, 0.9f);
    params.object_color = glm::vec3(0.8f, 0.85f, 0.95f);
    #pragma endregion

    // Define physical domain and discretization, setting up fields
    #pragma region
    Fields fields;

    // fields.air_mask = air_mask_slope_up(params, 0.0f, 100.0f);
    fields.air_mask = air_mask_flat(params, 25.0f);
    fields.snow_density = Field2D<float>(params.nx, params.ny);
    fields.next_snow_density = Field2D<float>(params.nx, params.ny);
    fields.snow_transport_speed_x = Field2D<float>(params.nx + 1, params.ny, params.wind_speed);
    fields.snow_transport_speed_y = Field2D<float>(params.nx, params.ny + 1, -params.settling_speed);
    fields.precipitation_source = Field1D<float>(params.nx, params.precipitation_rate);
    fields.windborn_horizontal_source_right = Field1D<float>(params.ny, 0.0f);
    // Inflow at x=0: match the supplied precipitation with the local vertical motion,
    // then scale by the upwind horizontal speed to get the steady-state lateral source term (g/(m^2*s)).
    // fields.windborn_horizontal_source_left = Field1D<float>(params.ny, 0.0f);
    // Field1D<float> boundary_base_flux(params.ny, 0.0f);
    // Field1D<float> boundary_arrival_top(params.ny, 0.0f);
    // Field1D<float> boundary_cross_time(params.ny, 0.0f);
    // {
    //     // TODO: if settling speed, wind, or precipitation change over time this precomputation must be recomputed.
    //     const float abs_settling_speed = std::fabs(params.settling_speed);
    //     // Precompute arrival timings so the boundary inflow ramps while the falling column passes through each row.
    //     for (std::size_t j = 0; j < params.ny; ++j)
    //     {
    //         const float vy_bottom = fields.snow_transport_speed_y(0, j);
    //         const float vy_top = fields.snow_transport_speed_y(0, j + 1);
    //         const float vy_avg = 0.5f * (vy_bottom + vy_top); // boundary vertical velocity (m/s)
    //         const float abs_vy = std::fabs(vy_avg);
    //         const float vy_mag = abs_vy > 1e-6f ? abs_vy : 1e-6f; // prevent divide-by-zero
    //         const float ux = fields.snow_transport_speed_x(0, j); // left boundary face (m/s)
    //         const float ux_in = ux > 0.0f ? ux : 0.0f; // only inflow contributes
    //         const float lateral_scale = ux_in / vy_mag; // dimensionless ratio of speeds
    //         const bool is_air = fields.air_mask(0, j) != 0;

    //         boundary_base_flux(j) = is_air ? params.precipitation_rate * lateral_scale : 0.0f; // g/(m^2*s)

    //         const float cell_top_y = (static_cast<float>(j) + 1.0f) * params.dy;
    //         const float distance_to_cell_top = std::max(0.0f, params.Ly - cell_top_y);
    //         boundary_arrival_top(j) =
    //             (abs_settling_speed > 1e-6f) ? distance_to_cell_top / abs_settling_speed : 0.0f; // snow reaches top of cell j

    //         boundary_cross_time(j) =
    //             (abs_settling_speed > 1e-6f) ? (params.dy / abs_settling_speed) : 0.0f; // time to traverse a full cell
    //     }
    // }
    // float elapsed_time = 0.0f;
    // setting up sorce left sorce vector and vector helper variables
    fields.windborn_horizontal_source_left = Field1D<float>(params.ny, 0.0f);
    Field1D<float> left_boundary_column_prev(params.ny, 0.0f);
    Field1D<float> left_boundary_column_next(params.ny, 0.0f);
    const float inverse_time_step = params.time_step_duration > 0.0f
        ? 1.0f / params.time_step_duration
        : 0.0f;

    fields.snow_accumulation_mass = Field1D<float>(params.nx);
    fields.snow_accumulation_density = Field1D<float>(params.nx,params.settaled_snow_density);

    #pragma endregion

    const bool viz_ready = viz::initialize(1280, 720, "SnowSim Preview");
    // const bool viz_ready = false;

    if (!viz_ready)
    {
        std::cerr << "[viz] Visualization disabled.\n";
    }
    else
    {
        viz::initialize_air_mask_resources(fields.air_mask.ny, fields.air_mask.nx);
        viz::initialize_arrow_resources(fields.air_mask.ny, fields.air_mask.nx);
    }

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
    // TODO: configure GLAD/OpenGL state for visualization once rendering is implemented 
    // TODO: if you need textures use stb_image.h not SOIL2. I know its what you did in class but its old AF.
    // sim loop
    
    // TODO: for debug remove when working
    // params.settling_speed = 0.0f;          // m/sec    
    // fields.snow_transport_speed_y = Field2D<float>(params.nx, params.ny + 1, 0);

    for (int t = 0; t < params.total_time_steps; ++t)
    {
        if (viz_ready)
        {
            viz::poll_events();
            viz::process_input();

            if(viz::should_close()){
                break;
            }

            if (t % params.steps_per_frame == 0)
            {
                viz::begin_frame();
                viz::render_frame(params, fields);
                viz::end_frame();
            }
        }

        // TODO: remove when not needed for debuging
        if (ceilf(t * params.time_step_duration / 60.0f) != ceilf((t + 1) * params.time_step_duration / 60.0f))    
        {
            std::cout << t*params.time_step_duration/60 << " min into sim\n"; 
            // std::cout << t*params.time_step_duration << " sec into sim\n"; 
            // std::cout << fields.snow_density(params.nx/2,params.ny/2) << " central snow density\n"; 
            if (int(ceilf(t * params.time_step_duration / 60.0f)) % 1 == 0)    
            {
                // std::cout << "total sim size = (" << params.nx << ", " << params.ny << ")\n"; 
                // std::cout << "density field 15 minutes into sim\n"; 
                print_field_subregion(fields.snow_density,0,20,0,20);
                // print_field_subregion(fields.next_snow_density,10,15,0,10);
                // print_field_subregion(fields.snow_transport_speed_x, 10, 20, 0, 10);
                // std::cout << "snow density at (20,10): " << fields.snow_density.idx(19,9) << "\n"; 
            }
        }

        // print_field_subregion(fields.snow_density,0,50,17,20);

        sim.step(fields, params);

        // TODO: break windborn_horizontal_source_left ramp up into its own function
        // Accumulate simulation time and adjust boundary inflow to mimic the infinite upwind snowfall column.
        // elapsed_time += params.time_step_duration;

        // for (std::size_t j = 0; j < params.ny; ++j)
        // {
        //     if (boundary_base_flux(j) <= 0.0f || elapsed_time < boundary_arrival_top(j)) //if snow has not reached row or the max sorce value is 0
        //     {
        //         continue;
        //     }
        //     if (elapsed_time >= boundary_arrival_top(j) + boundary_cross_time(j)) //if snow has reached the bottom of the row
        //     {
        //         fields.windborn_horizontal_source_left(j) = boundary_base_flux(j);
        //     }
        //     else //if snow has reached the top but not the bottom of the row
        //     {
        //         const float ramp = std::clamp((elapsed_time - boundary_arrival_top(j)) / boundary_cross_time(j), 0.0f, 1.0f);
        //         fields.windborn_horizontal_source_left(j) = boundary_base_flux(j) * ramp;
        //     }
        // }

        // incrementing/ramping left boundry sorce
        left_boundary_column_next = step_snow_source(left_boundary_column_prev,
                                                     params.settling_speed,
                                                     params.precipitation_rate,
                                                     params.dy,
                                                     params.time_step_duration);

        for (std::size_t j = 0; j < params.ny; ++j)
        {
            if (!fields.air_mask(0, j))
            {
                fields.windborn_horizontal_source_left(j) = 0.0f;
                continue;
            }

            const float column_density = left_boundary_column_next(j);
            if (params.wind_speed <= 0.0f || params.dx <= 0.0f)
            {
                fields.windborn_horizontal_source_left(j) = 0.0f;
                continue;
            }

            fields.windborn_horizontal_source_left(j) = params.wind_speed * column_density / params.dx;
        }

        left_boundary_column_prev = left_boundary_column_next;
    }

    // std::cout << "accumulated snow" << ":\n";
    // for (int i = 0; i < params.nx; i++) {
    //     std::cout << fields.snow_accumulation_mass(i) << "\n";
    // }
    // std::cout << "\n";

    if (viz_ready)
    {
        viz::shutdown();
    }

    std::cout << "Finished simulation steps: grid(" << params.nx << "x" << params.ny << ")\n";

}

