
#include "my_helper.hpp"

#include "types.hpp"

namespace snow{

Field2D<uint8_t> air_mask_flat(Params params, float distince_from_bottom){
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

Field2D<uint8_t> air_mask_slope_up(Params params, float distince_from_bottom_left, float distince_from_top_right)
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
Field2D<uint8_t> air_mask_parabolic(Params params, float distince_from_bottom_center, float distince_from_top_edge)
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
} // namespace snow