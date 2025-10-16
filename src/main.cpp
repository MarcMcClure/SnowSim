#include <algorithm>
#include <iostream>
#include <cmath>
#include <string>
#include <glm/glm/glm.hpp>
#include "types.hpp"
#include "my_helper.hpp"
#include "simulation.hpp"
#include "cpu_backend.hpp"
// Safe to include: provides CPU fallback when SNOWSIM_HAS_CUDA == 0
// and CUDA interface when enabled.
#include "cuda_backend.hpp"
#include "renderer.hpp"

int main(int argc, char* argv[])
{
    using namespace snow;

    Params params{};
    Fields fields;
    const std::string config_path = (argc > 1) ? argv[1] : std::string("resources/configs/default.json");
    if (!load_simulation_config(config_path, params, fields))
    {
        std::cerr << "[config] params and fields failed to load from file\n";
        return 1;
    }

    // setting up sorce left sorce helper variables
    Field1D<float> left_boundary_column_prev(params.ny, 0.0f);
    Field1D<float> left_boundary_column_next(params.ny, 0.0f);

    bool viz_ready = false;
    if (params.viz_on)
    {
        viz_ready = viz::initialize(1280, 720, "SnowSim Preview");
    }
    if (!viz_ready){
        std::cerr << "[viz] Visualization disabled.\n";
    }
    else{
        viz::initialize_air_mask_resources(params);
        viz::initialize_arrow_resources(params);
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
                viz::render_frame(params, fields);
                viz::end_frame();
            }
        }

        // DEBUG: remove when not needed for debuging
        if (ceilf(t * params.time_step_duration / 60.0f) != ceilf((t + 1) * params.time_step_duration / 60.0f))    
        {
            std::cout << t*params.time_step_duration/60 << " min into sim\n"; 
            if (int(ceilf(t * params.time_step_duration / 60.0f)) % 10 == 0)    
            {
                print_field_subregion(fields.snow_density,0,20,0,20);
            }
        }

        sim.step(fields, params);

        // incrementing/ramping left boundry sorce
        left_boundary_column_next = step_snow_source(left_boundary_column_prev,
                                                     params.settling_speed,
                                                     params.precipitation_rate,
                                                     params.dy,
                                                     params.time_step_duration);

        for (std::size_t j = 0; j < params.ny; ++j)
        {
            // if wind blows left at left boundery in row j, left most cell in row j is under ground, or cell width is 0 (safty check)
            if (fields.air_mask(0,j) <= 0.0f || params.dx <= 0.0f || !fields.air_mask(0, j)){
                fields.windborn_horizontal_source_left(j) = 0.0f;
            }
            else{
                fields.windborn_horizontal_source_left(j) = params.wind_speed * left_boundary_column_next(j) / params.dx;
            }
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

