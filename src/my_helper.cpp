
#include "my_helper.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "json.hpp"
#include "types.hpp"

namespace snow{

Field2D<uint8_t> air_mask_flat(const Params& params, float distince_from_bottom){
    //safty checks
    if(distince_from_bottom > params.Ly) distince_from_bottom = params.Ly;
    if(distince_from_bottom < 0) distince_from_bottom = 0;

    Field2D<uint8_t> air_mask = Field2D<uint8_t>(params.nx, params.ny, 1);
    std::size_t cells_from_bottom = static_cast<std::size_t>(distince_from_bottom / params.dy);
    for (std::size_t i = 0; i < air_mask.nx; i++){
        for (std::size_t j = 0; j < air_mask.ny; j++){
            if(j <= cells_from_bottom) air_mask(i,j) = 0;
            else continue;
        }
    }
    return air_mask;
}

Field2D<uint8_t> air_mask_slope_up(const Params& params, float distince_from_bottom_left, float distince_from_top_right)
{
    
    //safty checks
    if(distince_from_bottom_left > params.Ly) distince_from_bottom_left = params.Ly;
    if(distince_from_bottom_left < 0) distince_from_bottom_left = 0;
    if(distince_from_top_right > params.Ly) distince_from_top_right = params.Ly;
    if(distince_from_top_right < 0) distince_from_top_right = 0;
    
    Field2D<uint8_t> air_mask = Field2D<uint8_t>(params.nx, params.ny, 1);
    
    std::size_t cells_from_top = static_cast<std::size_t>(distince_from_top_right / params.dy);
    std::size_t cells_from_bottom = static_cast<std::size_t>(distince_from_bottom_left / params.dy);
    float sloap = static_cast<float>(params.ny-cells_from_bottom-cells_from_top)/ static_cast<float>(params.nx);
    for (std::size_t i = 0; i < air_mask.nx; i++){
        for (std::size_t j = 0; j < air_mask.ny; j++){
            if(static_cast<float>(j) <= static_cast<float>(cells_from_bottom) + sloap * static_cast<float>(i)) air_mask(i,j) = 0;
            else continue;
        }
    }
    return air_mask;
}
// TODO: change docs to reflect function inputs and outputs
// ground mask where the ground is a parabola with a minimum of params.ground ans a max of params.Ly - params.ground
Field2D<uint8_t> air_mask_parabolic(const Params& params, float distince_from_bottom_center, float distince_from_top_edge)
{
    
    //safty checks
    if(distince_from_bottom_center > params.Ly) distince_from_bottom_center = params.Ly;
    if(distince_from_bottom_center < 0) distince_from_bottom_center = 0;
    if(distince_from_top_edge > params.Ly) distince_from_top_edge = params.Ly;
    if(distince_from_top_edge < 0) distince_from_top_edge = 0;
    if(distince_from_top_edge + distince_from_bottom_center > params.Ly);
    
    Field2D<uint8_t> air_mask = Field2D<uint8_t>(params.nx, params.ny, 1);
    
    const float vertex_x = 0.5f * params.Lx;
    const float denom = vertex_x * vertex_x;
    const float a = denom > 0.0f ? (distince_from_top_edge - distince_from_bottom_center) / denom : 0.0f;

    for (std::size_t i = 0; i < air_mask.nx; ++i)
    {
        const float x_center = (static_cast<float>(i) + 0.5f) * params.dx;
        float ground_y = a * (x_center - vertex_x) * (x_center - vertex_x) + distince_from_bottom_center;

        for (std::size_t j = 0; j < air_mask.ny; ++j)
        {
            const float cell_center_y = (static_cast<float>(j) + 0.5f) * params.dy;
            if (cell_center_y <= ground_y)
            {
                air_mask(i, j) = 0;
            }
        }
    }
    return air_mask;
}

// Prints a rectangular slice of a Field2D with consistent formatting for debugging.
void print_field_subregion(const Field2D<float>& field,
                           std::ptrdiff_t x_min,
                           std::ptrdiff_t x_max,
                           std::ptrdiff_t y_min,
                           std::ptrdiff_t y_max)
{
    if (field.nx == 0 || field.ny == 0)
    {
        // Guard against empty fields so downstream code does not attempt to index them.
        std::cout << "[print_field_subregion] empty field\n";
        return;
    }

    // Reorder min/max inputs so lower bounds precede upper bounds before clamping.
    std::ptrdiff_t x_lower = std::min(x_min, x_max);
    std::ptrdiff_t x_upper = std::max(x_min, x_max);
    std::ptrdiff_t y_lower = std::min(y_min, y_max);
    std::ptrdiff_t y_upper = std::max(y_min, y_max);

    const std::ptrdiff_t max_x_index = static_cast<std::ptrdiff_t>(field.nx - 1);
    const std::ptrdiff_t max_y_index = static_cast<std::ptrdiff_t>(field.ny - 1);

    // Clamp the requested subregion to the valid grid extents.
    x_lower = std::clamp(x_lower, static_cast<std::ptrdiff_t>(0), max_x_index);
    x_upper = std::clamp(x_upper, static_cast<std::ptrdiff_t>(0), max_x_index);
    y_lower = std::clamp(y_lower, static_cast<std::ptrdiff_t>(0), max_y_index);
    y_upper = std::clamp(y_upper, static_cast<std::ptrdiff_t>(0), max_y_index);

    if (x_lower > x_upper || y_lower > y_upper)
    {
        // Degenerate subregions collapse to an empty printout.
        std::cout << "[print_field_subregion] empty range after clamping\n";
        return;
    }

    // Announce which subregion is being displayed.
    std::cout << "Field2D subregion x[" << x_lower << ", " << x_upper
              << "] y[" << y_lower << ", " << y_upper << "]\n";

    // Buffer formatted strings to compute column widths before printing.
    std::vector<std::vector<std::pair<bool, std::string>>> formatted_values;
    formatted_values.reserve(static_cast<std::size_t>(y_upper - y_lower + 1));

    std::size_t max_entry_width = 0;

    for (std::ptrdiff_t row = y_upper; row >= y_lower; --row)
    {
        // Store per-cell data for the current row so it can be printed later.
        std::vector<std::pair<bool, std::string>> row_values;
        row_values.reserve(static_cast<std::size_t>(x_upper - x_lower + 1));

        const std::size_t row_index = static_cast<std::size_t>(row);

        for (std::ptrdiff_t column = x_lower; column <= x_upper; ++column)
        {
            const std::size_t column_index = static_cast<std::size_t>(column);
            const float value = field(column_index, row_index);
            const bool is_zero = value == 0.0f;

            std::string formatted;
            if (!is_zero)
            {
                // Non-zero values use scientific notation with two significant figures.
                std::ostringstream stream;
                stream.setf(std::ios::scientific);
                stream << std::setprecision(1) << static_cast<double>(value);
                formatted = stream.str();
                max_entry_width = std::max(max_entry_width, formatted.size());
            }

            row_values.emplace_back(is_zero, std::move(formatted));
        }

        formatted_values.emplace_back(std::move(row_values));
    }

    if (max_entry_width == 0)
    {
        // When every cell is zero we still want a single-character column.
        max_entry_width = 1;
    }

    for (std::size_t row = 0; row < formatted_values.size(); ++row)
    {
        const std::ptrdiff_t actual_row = y_upper - static_cast<std::ptrdiff_t>(row);

        // print row headers
        int header_length = 10; 
        std::string unpadded = "y=" + std::to_string(actual_row) + " -> ";
        std::string padded = unpadded + std::string(header_length - unpadded.size(), ' ');
        std::cout << padded;

        const auto& row_values = formatted_values[row];
        for (std::size_t column = 0; column < row_values.size(); ++column)
        {
            const bool is_zero = row_values[column].first;
            const std::string& formatted = row_values[column].second;

            if (is_zero)
            {
                // Keep zero entries aligned with their scientific neighbors.
                std::cout << '0' << std::setw(static_cast<int>(max_entry_width));
            }
            else
            {
                std::cout << std::setw(static_cast<int>(max_entry_width)) << formatted;
            }

            if (column + 1u < row_values.size())
            {
                // Separating columns with a single space improves readability.
                std::cout << ' ';
            }
        }

        std::cout << '\n';
    }
    std::cout.flush();
}

Field1D<float> step_snow_source(const Field1D<float>& column_density,
                                float settling_speed,
                                float precipitation_rate,
                                float dy,
                                float time_step_duration)
{
    // Early-out when the input column is empty or the geometric/time scales are invalid.
    if (column_density.nx == 0 || dy <= 0.0f || time_step_duration <= 0.0f)
    {
        return column_density;
    }

    Field1D<float> next_column(column_density.nx, 0.0f);

    // Settling drives a constant downward velocity in this one-dimensional column.
    const float vertical_velocity = -settling_speed;
    if (vertical_velocity == 0.0f)
    {
        // Pure precipitation update with no vertical transport.
        for (std::size_t j = 0; j < column_density.nx; ++j)
        {
            float density = column_density(j);
            if (j + 1 == column_density.nx)
            {
                density += time_step_duration * precipitation_rate;
            }
            next_column(j) = std::max(density, 0.0f);
        }
        return next_column;
    }

    // lambda function that Match the face flux behaviour used in the CPU simulation 
    // so the boundary source evolves in lock-step with interior cells.
    const float flux_threshold = 1e-5f; // TODO: Share this threshold with other face_flux helpers in cpu_backend.cpp.
    const auto face_flux = [&](std::size_t face_index) -> float
    {
        std::ptrdiff_t donor_index = (vertical_velocity > 0.0f)
            ? static_cast<std::ptrdiff_t>(face_index) - 1
            : static_cast<std::ptrdiff_t>(face_index);

        if (donor_index < 0 ||static_cast<std::size_t>(donor_index) >= column_density.nx){
            return 0.0f;
        }

        const float flux = vertical_velocity * column_density(static_cast<std::size_t>(donor_index));
        return (std::fabs(flux) > flux_threshold) ? flux : 0.0f;
    };

    for (std::size_t j = 0; j < column_density.nx; ++j)
    {
        float density = column_density(j);

        // Only the top cell receives direct precipitation.
        if (j + 1 == column_density.nx)
        {
            density += time_step_duration * precipitation_rate;
        }

        const float flux_bottom = face_flux(j);
        const float flux_top = face_flux(j + 1);

        density += time_step_duration / dy * (flux_bottom - flux_top);
        next_column(j) = std::max(density, 0.0f);
    }

    return next_column;
}

bool load_simulation_config(const std::string& config_path,
                            Params& params_out,
                            Fields& fields_out)
{
    // Attempt to open the requested configuration file; fail fast if the path is invalid.
    std::ifstream config_stream(config_path);
    if (!config_stream)
    {
        return false;
    }

    nlohmann::json root;
    try
    {
        config_stream >> root;
    }
    catch (const nlohmann::json::parse_error&)
    {
        return false;
    }

    if (!root.contains("params") || !root["params"].is_object())
    {
        return false;
    }

    const auto& params_node = root["params"];
    try
    {
        // Populate every numeric member of Params directly from the JSON object.
        params_out.wind_speed = params_node["wind_speed"].get<float>();
        params_out.settling_speed = params_node["settling_speed"].get<float>();
        params_out.precipitation_rate = params_node["precipitation_rate"].get<float>();
        params_out.ground_height = params_node["ground_height"].get<float>();
        params_out.settaled_snow_density = params_node["settaled_snow_density"].get<float>();
        params_out.Lx = params_node["Lx"].get<float>();
        params_out.Ly = params_node["Ly"].get<float>();
        params_out.dx = params_node["dx"].get<float>();
        params_out.dy = params_node["dy"].get<float>();
        params_out.total_sim_time = params_node["total_sim_time"].get<float>();
        params_out.time_step_duration = params_node["time_step_duration"].get<float>();
        params_out.steps_per_frame = params_node["steps_per_frame"].get<int>();

        const auto light_direction_array = params_node["light_direction"];
        params_out.light_direction = glm::vec3(light_direction_array[0].get<float>(),
                                               light_direction_array[1].get<float>(),
                                               light_direction_array[2].get<float>());

        const auto light_color_array = params_node["light_color"];
        params_out.light_color = glm::vec3(light_color_array[0].get<float>(),
                                           light_color_array[1].get<float>(),
                                           light_color_array[2].get<float>());

        const auto object_color_array = params_node["object_color"];
        params_out.object_color = glm::vec3(object_color_array[0].get<float>(),
                                            object_color_array[1].get<float>(),
                                            object_color_array[2].get<float>());
        params_out.arrow_plane_z = params_node["arrow_plane_z"].get<float>();
        params_out.arrow_density_max = params_node["arrow_density_max"].get<float>();
        params_out.arrow_reference_wind = params_node["arrow_reference_wind"].get<float>();
        params_out.arrow_min_length = params_node["arrow_min_length"].get<float>();
        params_out.viz_on = params_node["viz_on"].get<bool>();

        params_out.nx = static_cast<std::size_t>(std::lround(params_out.Lx / params_out.dx));
        params_out.ny = static_cast<std::size_t>(std::lround(params_out.Ly / params_out.dy));

        params_out.total_time_steps = static_cast<int>(std::lround(params_out.total_sim_time / params_out.time_step_duration));
    }
    catch (const nlohmann::json::type_error&)
    {
        return false;
    }

    if (!root.contains("fields") || !root["fields"].is_object())
    {
        return false;
    }

    const auto& fields_node = root["fields"];

    // Helper to populate 2D float fields (snow densities, velocity grids, etc.).
    auto load_field2d = [](const nlohmann::json& node, Field2D<float>& field) -> bool
    {
        try
        {
            const std::size_t nx = node["nx"].get<std::size_t>();
            const std::size_t ny = node["ny"].get<std::size_t>();
            const auto& data = node["data"];
            if (!data.is_array() || data.size() != nx * ny)
            {
                return false;
            }

            field.resize(nx, ny, 0.0f);
            for (std::size_t idx = 0; idx < data.size(); ++idx)
            {
                field.data[idx] = data[idx].get<float>();
            }
        }
        catch (const nlohmann::json::type_error&)
        {
            return false;
        }
        return true;
    };

    // Helper to populate the uint8_t 2D air-mask.
    auto load_field2d_u8 = [](const nlohmann::json& node, Field2D<std::uint8_t>& field) -> bool
    {
        try
        {
            const std::size_t nx = node["nx"].get<std::size_t>();
            const std::size_t ny = node["ny"].get<std::size_t>();
            const auto& data = node["data"];
            if (!data.is_array() || data.size() != nx * ny)
            {
                return false;
            }

            field.resize(nx, ny, static_cast<std::uint8_t>(0));
            for (std::size_t idx = 0; idx < data.size(); ++idx)
            {
                field.data[idx] = data[idx].get<std::uint8_t>();
            }
        }
        catch (const nlohmann::json::type_error&)
        {
            return false;
        }
        return true;
    };

    // Helper to populate 1D float fields (boundary sources, accumulation arrays, etc.).
    auto load_field1d = [](const nlohmann::json& node, Field1D<float>& field) -> bool
    {
        try
        {
            const std::size_t nx = node["nx"].get<std::size_t>();
            const auto& data = node["data"];
            if (!data.is_array() || data.size() != nx)
            {
                return false;
            }

            field.resize(nx, 0.0f);
            for (std::size_t idx = 0; idx < data.size(); ++idx)
            {
                field.data[idx] = data[idx].get<float>();
            }
        }
        catch (const nlohmann::json::type_error&)
        {
            return false;
        }
        return true;
    };

    // if (!load_field2d_u8(fields_node["air_mask"], fields_out.air_mask)) return false;
    // if (!load_field2d(fields_node["snow_density"], fields_out.snow_density)) return false;
    // if (!load_field2d(fields_node["next_snow_density"], fields_out.next_snow_density)) return false;
    // if (!load_field2d(fields_node["snow_transport_speed_x"], fields_out.snow_transport_speed_x)) return false;
    // if (!load_field2d(fields_node["snow_transport_speed_y"], fields_out.snow_transport_speed_y)) return false;
    // if (!load_field1d(fields_node["snow_accumulation_mass"], fields_out.snow_accumulation_mass)) return false;
    // if (!load_field1d(fields_node["snow_accumulation_density"], fields_out.snow_accumulation_density)) return false;
    // if (!load_field1d(fields_node["precipitation_source"], fields_out.precipitation_source)) return false;
    // if (!load_field1d(fields_node["windborn_horizontal_source_left"], fields_out.windborn_horizontal_source_left)) return false;
    // if (!load_field1d(fields_node["windborn_horizontal_source_right"], fields_out.windborn_horizontal_source_right)) return false;

    fields_out.air_mask = air_mask_flat(params_out, 25.0f);
    fields_out.snow_density = Field2D<float>(params_out.nx, params_out.ny);
    fields_out.next_snow_density = Field2D<float>(params_out.nx, params_out.ny);
    fields_out.snow_transport_speed_x = Field2D<float>(params_out.nx + 1, params_out.ny, params_out.wind_speed);
    fields_out.snow_transport_speed_y = Field2D<float>(params_out.nx, params_out.ny + 1, -params_out.settling_speed);
    fields_out.precipitation_source = Field1D<float>(params_out.nx, params_out.precipitation_rate);
    fields_out.windborn_horizontal_source_left = Field1D<float>(params_out.ny, 0.0f);
    fields_out.windborn_horizontal_source_right = Field1D<float>(params_out.ny, 0.0f);
    fields_out.snow_accumulation_mass = Field1D<float>(params_out.nx);
    fields_out.snow_accumulation_density = Field1D<float>(params_out.nx,params_out.settaled_snow_density);

    return true;
}

void dump_simulation_state_to_example_json(const Params& params,
                                           const Fields& fields)
{
    nlohmann::json root;

    nlohmann::json params_node;
    params_node["wind_speed"] = params.wind_speed;
    params_node["settling_speed"] = params.settling_speed;
    params_node["precipitation_rate"] = params.precipitation_rate;
    params_node["ground_height"] = params.ground_height;
    params_node["settaled_snow_density"] = params.settaled_snow_density;
    params_node["Lx"] = params.Lx;
    params_node["Ly"] = params.Ly;
    params_node["dx"] = params.dx;
    params_node["dy"] = params.dy;
    params_node["nx"] = params.nx;
    params_node["ny"] = params.ny;
    params_node["total_sim_time"] = params.total_sim_time;
    params_node["time_step_duration"] = params.time_step_duration;
    params_node["total_time_steps"] = params.total_time_steps;
    params_node["steps_per_frame"] = params.steps_per_frame;
    params_node["light_direction"] = nlohmann::json::array({ params.light_direction.x,
                                                             params.light_direction.y,
                                                             params.light_direction.z });
    params_node["light_color"] = nlohmann::json::array({ params.light_color.x,
                                                         params.light_color.y,
                                                         params.light_color.z });
    params_node["object_color"] = nlohmann::json::array({ params.object_color.x,
                                                          params.object_color.y,
                                                          params.object_color.z });
    root["params"] = params_node;

    nlohmann::json fields_node;
    fields_node["air_mask"] = {
        { "nx", fields.air_mask.nx },
        { "ny", fields.air_mask.ny },
        { "data", fields.air_mask.data }
    };
    fields_node["snow_density"] = {
        { "nx", fields.snow_density.nx },
        { "ny", fields.snow_density.ny },
        { "data", fields.snow_density.data }
    };
    fields_node["next_snow_density"] = {
        { "nx", fields.next_snow_density.nx },
        { "ny", fields.next_snow_density.ny },
        { "data", fields.next_snow_density.data }
    };
    fields_node["snow_transport_speed_x"] = {
        { "nx", fields.snow_transport_speed_x.nx },
        { "ny", fields.snow_transport_speed_x.ny },
        { "data", fields.snow_transport_speed_x.data }
    };
    fields_node["snow_transport_speed_y"] = {
        { "nx", fields.snow_transport_speed_y.nx },
        { "ny", fields.snow_transport_speed_y.ny },
        { "data", fields.snow_transport_speed_y.data }
    };
    fields_node["snow_accumulation_mass"] = {
        { "nx", fields.snow_accumulation_mass.nx },
        { "data", fields.snow_accumulation_mass.data }
    };
    fields_node["snow_accumulation_density"] = {
        { "nx", fields.snow_accumulation_density.nx },
        { "data", fields.snow_accumulation_density.data }
    };
    fields_node["precipitation_source"] = {
        { "nx", fields.precipitation_source.nx },
        { "data", fields.precipitation_source.data }
    };
    fields_node["windborn_horizontal_source_left"] = {
        { "nx", fields.windborn_horizontal_source_left.nx },
        { "data", fields.windborn_horizontal_source_left.data }
    };
    fields_node["windborn_horizontal_source_right"] = {
        { "nx", fields.windborn_horizontal_source_right.nx },
        { "data", fields.windborn_horizontal_source_right.data }
    };
    root["fields"] = fields_node;

    std::ofstream output_stream("resources/configs/example1.json");
    output_stream << root.dump(2);
}
} // namespace snow
