#include <string>

#include "catch_amalgamated.hpp"
#include "my_helper.hpp"
#include "support/my_catch_warning.hpp"

using snow::Field1D;
using test_support::capture_stderr;

//tests step_snow_source in my_helper.json
// TODO: make error checking assertions prittier, take out the nextr anew lines.

namespace {
    const Field1D<float> column_density(5, 10.0f);
    const float settling_speed = 0.5f;
    const float precipitation_rate = 0.1f;
    const float dy = 10.0f;
    const float dt = 0.1f;
}

TEST_CASE("step_snow_source baseline evolution", "[snow_source][unit]")
{
    const Field1D<float> next_column_density = snow::step_snow_source(column_density,
                                                              settling_speed,
                                                              precipitation_rate,
                                                              dy,
                                                              dt);
    const Field1D<float> expected_column_density({10.0f,10.0f,10.0f,10.0f,9.96f});
    REQUIRE(next_column_density.nx == column_density.nx );
    REQUIRE_THAT(next_column_density.data, Catch::Matchers::Approx(expected_column_density.data).margin(1e-5f));
}

TEST_CASE("step_snow_source column size zero", "[snow_source][unit][warning]")
{
    const Field1D<float> column_density_zero(0, 10.0f);

    Field1D<float> next_column_density;
    const std::string warning = capture_stderr([&] {
        next_column_density = snow::step_snow_source(column_density_zero,
                                                     settling_speed,
                                                     precipitation_rate,
                                                     dy,
                                                     dt);
    });

    REQUIRE(warning == "Warning: step_snow_source received cell number/column_density.nx == 1\n");
    REQUIRE(next_column_density.nx == column_density_zero.nx);
    REQUIRE(next_column_density.data == column_density_zero.data);
}

/**
we are now planning

ok so lets start with snow sorce test. here are teh test i can think of so far, with a baseline params of seomthing like this 
Note: all checks buse pass a CLF check: time_step_duration / dy  * abs(settling_speed) < 1
baseline params:

const Field1D<float>& column_density = Field1D(5, 10), 
float settling_speed = 0.5, 
float precipitation_rate =  0.1
float dy = 10,
float time_step_duration = 0.1,

run the baseline test then test with the following input variations, grouped by focus:

baseline test

warning checks (check both the output vaiable and std::cerr)
    column_density = Field1D(0, 0) 
    dy = -1
    dy = 0
    dt = -1    
    dt = 0
    precipitation_rate = -1   

    clf warning checks:
        dy = 0.0001
        dt = 99999
        settling_speed = 99999
        settling_speed = -99999

dy, dt, and settling_speed:
    dt = 0.00001
    dy= 0.0001, dt = 0.00001,
    settling_speed = 0.00001,
    dy= 0.0001, settling_speed = 0.00001,
    settling_speed = -0.00001
    dy= 0.0001, settling_speed = -0.00001,
    dy= 99999
    dy= 99999, dt = 9999,
    dy= 99999, settling_speed = 9999,
    dy= 99999, settling_speed = -9999,
    settling_speed = 0

precipitation_rate:
    precipitation_rate = -1
    precipitation_rate = 0
    precipitation_rate = 99999
    
column_density:
    column_density = Field1D(5, -1)
    column_density = Field1D(1, -0.000001)
    column_density = Field1D(5, 0)
    column_density = Field1D(1, 0.000001)
    column_density = Field1D(5, 0.1)
    column_density = Field1D(5, 99999)

    column_density = Field1D(1, 1)
    column_density = Field1D(9999, 1)
 */
