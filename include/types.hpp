// Basic types used across the simulation
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <glm/glm/glm.hpp>

namespace snow
{

    struct Params
    {
        float wind_speed;           // m/sec
        float settling_speed;       // m/sec
        float precipitation_rate;   // g/m^2/s
        float ground_height;        // m above y=0
        float settaled_snow_density; // g/m^2

        float Lx; // physical width in meters (x direction)
        float Ly; // physical height in meters (y direction)        
        float dx; // cell size in x (meters/cell)
        float dy; // cell size in y (meters/cell)

        std::size_t nx; // number of cells in x (computed)
        std::size_t ny; // number of cells in y (computed)

        float total_sim_time;     // in sec
        float time_step_duration; // in sec

        int total_time_steps; // number of steps

        int steps_per_frame;

        glm::vec3 light_direction; // world-space direction toward the light
        glm::vec3 light_color;     // RGB intensity for the directional light
        glm::vec3 object_color;    // base color applied to rendered geometry
    };

    template <typename T>
    struct Field1D
    {
        std::size_t nx{}; // num of cells

        std::vector<T> data; //vect for data storage

        Field1D() = default; // default constructor

        // construct Field1D of size nx_ and a unifrom value that defaults to type spicfic 0
        Field1D(std::size_t nx_, const T& uniform_field_value = T{}) :
            nx(nx_),
            data(nx_, uniform_field_value)
        {}

        // Element access (mutable)
        inline T& operator()(std::size_t i)
        {
            return data[i];
        }

        // Element access (const)
        inline const T& operator()(std::size_t i) const
        {
            return data[i];
        }

        // Resize the field to nx_ and fill all entries with 'init'.
        inline void resize(std::size_t nx_, const T& init = T{})
        {
            nx = nx_;
            data.assign(nx, init);
        }

        // helper to check if i is inbounds
        inline bool in_bounds(std::size_t i) const
        {
            return i < nx;
        }
    };

    // Field2D: simple 2D array wrapper with flat (row-major) storage.
    // - T: element type (e.g., float, uint8_t)
    // - Indexing convention: (i, j) where i is x (column), j is y (row)
    // - Memory layout: data[j * nx + i]
    template <typename T>
    struct Field2D
    {
        // Logical grid size (number of cells in each direction)
        std::size_t nx{}; // cells in x
        std::size_t ny{}; // cells in y

        // Contiguous storage for nx*ny elements, row-major (y-major rows)
        std::vector<T> data;

        // Default constructor: creates an empty field (nx = ny = 0)
        Field2D() = default;

        // Sized constructor: allocate nx_*ny_ elements and fill with 'init'.
        Field2D(std::size_t nx_, std::size_t ny_, const T& uniform_field_value = T{}) : 
            nx(nx_), 
            ny(ny_),
            data(nx_ * ny_, uniform_field_value) 
        {}

        // Convert (i, j) to flat index into 'data'.
        // Precondition: 0 <= i < nx and 0 <= j < ny.
        inline std::size_t idx(std::size_t i, std::size_t j) const
        {
            return j * nx + i;
        }

        // Element access (mutable)
        inline T& operator()(std::size_t i, std::size_t j)
        {
            return data[idx(i, j)];
        }

        // Element access (const)
        inline const T& operator()(std::size_t i, std::size_t j) const
        {
            return data[idx(i, j)];
        }

        // Resize the field to nx_*ny_ and fill all entries with 'init'.
        inline void resize(std::size_t nx_, std::size_t ny_, const T& init = T{})
        {
            nx = nx_;
            ny = ny_;
            data.assign(nx * ny, init);
        }

        // Bounds check helper for debug/asserts or conditional access.
        inline bool in_bounds(std::size_t i, std::size_t j) const
        {
            return i < nx && j < ny;
        }
    };

    //wind speeds are shifted left and down respectivly such that the edges suroudning snow_density(x,y)
    //will be at wind_speed_x(x,y) [left],wind_speed_x(x+1,y) [right],wind_speed_y(x,y) [bottom], wind_speed_y(x,y+1) [top].
    //wind is positive when blowing to the right and up.
    struct Fields
    {
        Field2D<std::uint8_t> air_mask;     // 1 = air, 0 = land
        Field2D<float> snow_density;        // snow density or concentration         g/m^2
        Field2D<float> next_snow_density;   // scratch buffer for next-step density     g/m^2
        Field2D<float> snow_transport_speed_x;        // wind x-component                      m/s
        Field2D<float> snow_transport_speed_y;        // wind y-component                      m/s
        Field1D<float> snow_accumulation_mass;   // accumulated snow mass on ground       g
        Field1D<float> snow_accumulation_density;   // on ground                     g/m^2
        Field1D<float> precipitation_source;  // rate of precipitation                 g/m^2/s
        Field1D<float> windborn_horizontal_source_left;// rate at which snow flows in from x=0  g/m^2/s
        Field1D<float> windborn_horizontal_source_right;// rate at which snow flows in from x=nx  g/m^2/s
    };

} // namespace snow

