#pragma once

#include <cstddef>

#include "types.hpp"

namespace snow
{

Field2D<uint8_t> air_mask_flat(const Params& params, float distince_from_bottom);
Field2D<uint8_t> air_mask_slope_up(const Params& params, float distince_from_bottom, float distince_from_top);
Field2D<uint8_t> air_mask_parabolic(const Params& params, float distince_from_bottom, float distince_from_top);
void print_field_subregion(const Field2D<float>& field,
                           std::ptrdiff_t x_min,
                           std::ptrdiff_t x_max,
                           std::ptrdiff_t y_min,
                           std::ptrdiff_t y_max);

// Advances a one-dimensional snow column by a single time step so that the
// left-boundary source matches the settling/precipitation behaviour used by the
// main simulation loop.
Field1D<float> step_snow_source(const Field1D<float>& column_density,
                                float settling_speed,
                                float precipitation_rate,
                                float dy,
                                float time_step_duration);

} // namespace snow
