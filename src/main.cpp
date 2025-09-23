#include <algorithm>
#include <iostream>
#include <cmath>
#include <glm/glm/glm.hpp>
#include "types.hpp"
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

    params.total_sim_time = 1.0f  * 3600.0f;    //sec
    params.time_step_duration = 0.2f;           //sec
    params.steps_per_frame = 100;

    params.total_time_steps = static_cast<int>(std::lround(params.total_sim_time / params.time_step_duration));

    params.light_direction = glm::vec3(-0.4f, -1.0f, -0.6f);
    params.light_color = glm::vec3(1.0f, 0.95f, 0.9f);
    params.object_color = glm::vec3(0.8f, 0.85f, 0.95f);
    #pragma endregion

    // Define physical domain and discretization, setting up fields
    #pragma region
    Fields fields;
    fields.air_mask = Field2D<uint8_t>(params.nx, params.ny, 1);

    // ground mask where the ground is a parabola with a minimum of params.ground ans a max of params.Ly - params.ground
    // const float vertex_x = 0.5f * params.Lx;
    // const float vertex_y = params.ground_height;
    // const float edge_height = params.Ly - params.ground_height;
    // const float denom = vertex_x * vertex_x;
    // const float a = denom > 0.0f ? (edge_height - vertex_y) / denom : 0.0f;

    // for (std::size_t i = 0; i < fields.air_mask.nx; ++i)
    // {
    //     const float x_center = (static_cast<float>(i) + 0.5f) * params.dx;
    //     const float ground_y = a * (x_center - vertex_x) * (x_center - vertex_x) + vertex_y;

    //     for (std::size_t j = 0; j < fields.air_mask.ny; ++j)
    //     {
    //         const float cell_center_y = (static_cast<float>(j) + 0.5f) * params.dy;
    //         if (cell_center_y <= ground_y)
    //         {
    //             fields.air_mask(i, j) = 0;
    //         }
    //     }
    // }

    // ground mask where the ground is a flat plain at params.ground level
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

    const bool viz_ready = viz::initialize(1280, 720, "SnowSim Preview");
    if (!viz_ready)
    {
        std::cerr << "[viz] Visualization disabled.\n";
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
                viz::render_frame(params);
                viz::end_frame();
            }
        }

        // TODO: remove when not needed for debuging
        if (ceilf(t * params.time_step_duration / 60.0f) != ceilf((t + 1) * params.time_step_duration / 60.0f))    
        {
            std::cout << t*params.time_step_duration/60 << " min into sim\n"; 
        }

        sim.step(fields, params);
    }

    std::cout << "accumulated snow" << ":\n";
    for (int i = 0; i < params.nx; i++) {
        std::cout << fields.snow_accumulation_mass.data[i] << "\n";
    }
    std::cout << "\n";

    if (viz_ready)
    {
        viz::shutdown();
    }

    std::cout << "Finished simulation steps: grid(" << params.nx << "x" << params.ny << ")\n";

}

