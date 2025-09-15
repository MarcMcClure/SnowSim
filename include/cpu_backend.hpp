#pragma once

#include "simulation.hpp" // ensures Simulation base is defined

namespace snow
{
    namespace cpu
    {

        class CPUSimulation : public Simulation
        {
        public:
            void step(Grid &grid, const Params &params) override;
        };

    } // namespace cpu
} // namespace snow