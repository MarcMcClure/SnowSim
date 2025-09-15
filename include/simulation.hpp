#pragma once

#include <vector>
#include "types.hpp"

namespace snow
{

    class Simulation
    {
    public:
        virtual void step(Grid &grid, const Params &params) = 0;
        virtual ~Simulation() = default;
    };

} // namespace snow
