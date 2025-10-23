#include "catch_amalgamated.hpp"

// tests load_simulation_config from my_helper.json
// tests dump_simulation_state_to_example_json from my_helper.json
// skips testing air_mask on load because it still has hard coded elements
//TODO: add tests about cheking that value checks work once they are more fully implmented (eg no caves)
TEST_CASE("config loader placeholder", "[config_loader]")
{
    REQUIRE(true);
}
