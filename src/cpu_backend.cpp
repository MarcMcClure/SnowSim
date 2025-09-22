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
                const float velocity = fields.snow_transport_speed_x(face_i, j);
                if (velocity == 0.0f) return 0.0f; //no wind, return 0

                // source cell aligns with the upwind tile; mark out-of-domain with nx sentinel.
                const std::size_t source_cell_i = velocity > 0.0f
                    ? (face_i == 0 ? fields.snow_density.nx : face_i - 1)
                    : (face_i >= fields.snow_density.nx ? fields.snow_density.nx : face_i);

                if (source_cell_i >= fields.snow_density.nx) return 0.0f; // donor outside domain, return 0
                if (fields.air_mask(source_cell_i, j) == 0) return 0.0f; // donor is ground, return 0

                return velocity * fields.snow_density(source_cell_i, j);
            }
            // Upwind snow mass flux across horizontal face (i, face_j).
            // i: column index of the face (0..nx-1), face_j: row index (0..ny).
            // Returns g/(m*s) using the donor air cell; zero for ground or domain edges.
            inline float face_flux_y(const Fields& fields, std::size_t i, std::size_t face_j)
            {
                const float velocity = fields.snow_transport_speed_y(i, face_j);
                if (velocity == 0.0f) return 0.0f; //no wind, return 0

                // source cell aligns with the upwind tile; mark out-of-domain with ny sentinel.
                const std::size_t source_cell_j = velocity > 0.0f
                    ? (face_j == 0 ? fields.snow_density.ny : face_j - 1)
                    : (face_j >= fields.snow_density.ny ? fields.snow_density.ny : face_j);

                if (source_cell_j >= fields.snow_density.ny) return 0.0f; // donor outside domain, return 0
                if (fields.air_mask(i, source_cell_j) == 0) return 0.0f;// donor is ground, return 0

                return velocity * fields.snow_density(i, source_cell_j);
            }        } // namespace

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

                    if (i == 0 && fields.windborn_horizontal_source.in_bounds(j)) //if grid cell is in first col add snow from wind outside of sim
                    {
                        density += dt * fields.windborn_horizontal_source(j);
                    }

                    if (j == fields.snow_density.ny - 1 && fields.precipitation_source.in_bounds(i)) //if grid cell is in top row add snow from percipitation
                    {
                        density += dt * fields.precipitation_source(i);
                    }

                    //calculate snow flux on each side of the cell
                    const float flux_w = face_flux_x(fields, i, j);
                    const float flux_e = face_flux_x(fields, i + 1, j);
                    const float flux_s = face_flux_y(fields, i, j);
                    const float flux_n = face_flux_y(fields, i, j + 1);

                    //if grid cell is just above the ground and there is a negitive flux between the grid cell and the ground cell, deposit some snow onto the ground.
                    if (flux_s < 0.0f)
                    {
                        const bool ground_below = (j == 0) || (!fields.air_mask(i, j - 1));
                        if (ground_below)
                        {
                            const float deposit_per_area = (-flux_s) * dt / dy;
                            const float deposit_mass = deposit_per_area * dx;
                            column_deposit[i] += deposit_mass;
                        }
                    }

                    density -= (dt / dx) * (flux_e - flux_w);
                    density -= (dt / dy) * (flux_n - flux_s);

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














