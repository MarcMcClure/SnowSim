#include "cpu_backend.hpp"

#include <algorithm>
#include <vector>

namespace snow
{
    namespace cpu
    {

        namespace
        {
            // Upwind snow mass flux across vertical face (face_i, j).
            // face_i: column index of the face (0..nx), j: row index (0..ny-1).
            // Returns g/(m*s) using the donor air cell; zero for ground or domain edges.
            inline float face_flux_x(const Fields& fields, std::size_t face_i, std::size_t j)
            {
                // TODO: pull out threshold flux and put it in params so it can be consistant between face_flux_x and face_flux_y
                const float threshold_flux = 1e-5f;

                const float velocity = fields.snow_transport_speed_x(face_i, j);
                if (velocity == 0.0f) return 0.0f; //no wind, return 0

                // source cell aligns with the upwind tile; mark out-of-domain with nx sentinel.
                const std::size_t source_cell_i = (velocity > 0.0f)
                    ? face_i - 1 // positive velocity source cell is to the left
                    : face_i; // negitive velocity source cell is to the right

                if (!fields.snow_density.in_bounds(source_cell_i,j)) return 0.0f; // donor outside domain, return 0
                if (!fields.air_mask(source_cell_i, j)) return 0.0f; // donor is ground, return 0

                float face_flux = velocity * fields.snow_density(source_cell_i, j);
                const float clamped_face_flux = std::fabs(face_flux) > threshold_flux ? face_flux : 0.0f;

                return clamped_face_flux;
            }

            // Upwind snow mass flux across horizontal face (i, face_j).
            // i: column index of the face (0..nx-1), face_j: row index (0..ny).
            // Returns g/(m*s) using the donor air cell; zero for ground or domain edges.
            inline float face_flux_y(const Fields& fields, std::size_t i, std::size_t face_j)
            {
                const float threshold_flux = 1e-5f; // TODO: share with face_flux_x via params.

                const float velocity = fields.snow_transport_speed_y(i, face_j);
                if (velocity == 0.0f) return 0.0f; // no vertical wind, return 0

                // source cell aligns with the upwind tile; mark out-of-domain with ny sentinel.
                const std::size_t source_cell_j = (velocity > 0.0f)
                    ? face_j - 1 // positive velocity source cell is below
                    : face_j;    // negative velocity source cell is above

                if (!fields.snow_density.in_bounds(i, source_cell_j)) return 0.0f; // donor outside domain, return 0
                if (!fields.air_mask(i, source_cell_j)) return 0.0f; // donor is ground, return 0

                const float face_flux = velocity * fields.snow_density(i, source_cell_j);
                const float clamped_face_flux = (std::fabs(face_flux) > threshold_flux) ? face_flux : 0.0f;

                return clamped_face_flux;
            }        
        } // namespace

        void CPUSimulation::step(Fields& fields, const Params& params)
        {
            const float dt = params.time_step_duration;
            const float dx = params.dx;
            const float dy = params.dy;

            if (fields.next_snow_density.nx != fields.snow_density.nx || fields.next_snow_density.ny != fields.snow_density.ny) //checks if sim sizes don't match. this should alwasy be flase.
            {
                fields.next_snow_density.resize(fields.snow_density.nx, fields.snow_density.ny, 0.0f);
            }

            std::vector<float> column_deposit(fields.snow_density.nx, 0.0f);

            for (std::size_t j = 0; j < fields.snow_density.ny; ++j)
            {
                for (std::size_t i = 0; i < fields.snow_density.nx; ++i)
                {
                    if (!fields.air_mask(i, j)) // if grid cell is underground, it contains no snow.
                    {
                        fields.next_snow_density(i, j) = 0.0f;
                        continue;
                    }

                    float density = fields.snow_density(i, j);

                    float top_sorce = 0;
                    float right_sorce = 0;
                    float left_sorce = 0;
                    if (i == 0 && fields.windborn_horizontal_source_left.in_bounds(j)) //if grid cell is in left most col add snow from wind outside of sim
                    {
                        if(fields.snow_transport_speed_x.idx(i,j) > 0) // if snow is advecting in from the left
                            left_sorce =  fields.windborn_horizontal_source_left(j);
                    }

                    if (i == fields.snow_density.nx - 1 && fields.windborn_horizontal_source_right.in_bounds(j)) //if grid cell is in right most col add snow from wind outside of sim
                    {
                        if(fields.snow_transport_speed_x.idx(i+1,j) > 0) // if snow is advecting in from the right
                            right_sorce = fields.windborn_horizontal_source_right(j);
                    }

                    if (j == fields.snow_density.ny - 1 && fields.precipitation_source.in_bounds(i)) //if grid cell is in top row add snow from percipitation
                    {                        
                        if(fields.snow_transport_speed_x.idx(i,j+1) > 0) // if snow is advecting down from above
                            top_sorce = fields.precipitation_source(i);
                    }

                    //calculate snow flux on each side of the cell, velocity is positive when it is right or up
                    const float flux_left = face_flux_x(fields, i, j);
                    const float flux_right = face_flux_x(fields, i + 1, j);
                    const float flux_bottom = face_flux_y(fields, i, j);
                    const float flux_top = face_flux_y(fields, i, j + 1);

                    //if grid cell is just above the ground and there is a negitive flux between the grid cell and the ground cell, deposit some snow onto the ground.
                    if (flux_bottom < 0.0f)
                    {
                        const bool ground_below = (j == 0) || (!fields.air_mask(i, j - 1));
                        if (ground_below)
                        {
                            const float deposit_per_area = (-flux_bottom) * dt / dy;
                            const float deposit_mass = deposit_per_area * dx;
                            column_deposit[i] += deposit_mass;
                        }
                    }

                    density += (dt / dx) * (flux_left - flux_right);
                    density += (dt / dy) * (flux_bottom - flux_top);
                    density += dt * (left_sorce + right_sorce + top_sorce);

                    fields.next_snow_density(i, j) = std::max(density, 0.0f);
                }
            }

            std::swap(fields.snow_density, fields.next_snow_density);

            for (std::size_t i = 0; i < column_deposit.size(); ++i)
            {
                if (fields.snow_accumulation_mass.in_bounds(i))
                {
                    fields.snow_accumulation_mass(i) += column_deposit[i];
                }
            }
        }

    } // namespace cpu
} // namespace snow














