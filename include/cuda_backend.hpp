// CUDA backend interface with CPU fallback
#pragma once

#include "simulation.hpp"

#if SNOWSIM_HAS_CUDA

namespace snow {
namespace cuda {

class CUDASimulation : public Simulation {
public:
    void step(Fields& fields, const Params& params) override;
};

} // namespace cuda
} // namespace snow

#else // SNOWSIM_HAS_CUDA == 0

#include "cpu_backend.hpp"

namespace snow {
namespace cuda {

// Fallback: alias CUDA simulation to CPU implementation
using CUDASimulation = snow::cpu::CPUSimulation;

} // namespace cuda
} // namespace snow

#endif // SNOWSIM_HAS_CUDA
