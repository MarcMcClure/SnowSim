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

} // namespace snow
