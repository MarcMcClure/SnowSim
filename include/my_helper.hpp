#pragma once

#include "types.hpp"

namespace snow
{

Field2D<uint8_t> air_mask_flat(const Params& params, float distince_from_bottom);
Field2D<uint8_t> air_mask_slope_up(Params params, float distince_from_bottom, float distince_from_top);
Field2D<uint8_t> air_mask_parabolic(const Params& params, float distince_from_bottom, float distince_from_top);

} // namespace snow
