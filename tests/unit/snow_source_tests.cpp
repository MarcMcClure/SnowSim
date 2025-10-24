#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "catch_amalgamated.hpp"
#include "my_helper.hpp"
#include "support/my_catch_warning.hpp"

#include "json.hpp"

using snow::Field1D;
using test_support::capture_stderr;

namespace {
    struct SnowSourceScenario {
        std::string name;
        std::vector<std::string> tags;

        Field1D<float> column_density;
        float settling_speed{};
        float precipitation_rate{};
        float dy{};
        float dt{};

        Field1D<float> expected_column_density;
        std::string expected_warning;
    };

    std::vector<SnowSourceScenario> load_scenarios() {
        const std::filesystem::path path =
#ifdef SNOW_SOURCE_SCENARIO_FILE_PATH
            std::filesystem::path{ SNOW_SOURCE_SCENARIO_FILE_PATH };
#else
            std::filesystem::path{ "tests/unit/snow_source_scenarios.json" };
#endif

        std::ifstream json_stream(path);
        if (!json_stream) {
            throw std::runtime_error("Failed to open snow source scenario file: " + path.string());
        }

        nlohmann::json doc;
        json_stream >> doc;

        std::vector<SnowSourceScenario> scenarios;
        for (const auto& entry : doc.at("scenarios")) {
            SnowSourceScenario scenario{};
            scenario.name = entry.at("name").get<std::string>();
            scenario.tags = entry.value("tags", std::vector<std::string>{});

            const auto& inputs = entry.at("inputs");
            scenario.column_density =
                Field1D<float>(inputs.at("column_density").get<std::vector<float>>());
            scenario.settling_speed = inputs.at("settling_speed").get<float>();
            scenario.precipitation_rate = inputs.at("precipitation_rate").get<float>();
            scenario.dy = inputs.at("dy").get<float>();
            scenario.dt = inputs.at("dt").get<float>();

            scenario.expected_column_density =
                Field1D<float>(entry.at("expected_column_density").get<std::vector<float>>());
            scenario.expected_warning = entry.at("expected_warning").get<std::string>();

            scenarios.push_back(std::move(scenario));
        }

        return scenarios;
    }
}

TEST_CASE("step_snow_source scenarios", "[snow_source][unit][warning]") {
    const std::vector<SnowSourceScenario> scenarios = load_scenarios();

    for (const auto& scenario : scenarios) {
        DYNAMIC_SECTION("scenario: " << scenario.name) {
            Field1D<float> next_column_density;
            const std::string warning = capture_stderr([&] {
                next_column_density = snow::step_snow_source(
                    scenario.column_density,
                    scenario.settling_speed,
                    scenario.precipitation_rate,
                    scenario.dy,
                    scenario.dt);
            });

            REQUIRE(warning == scenario.expected_warning);
            REQUIRE(next_column_density.nx == scenario.expected_column_density.nx);
            REQUIRE_THAT(next_column_density.data, Catch::Matchers::Approx(scenario.expected_column_density.data).margin(1e-5f));
        }
    }
}


/**
we are now planning
ok so lets start with snow sorce test. here are teh test i can think of so far, with a baseline params of seomthing like this 
Note: all checks buse pass a CLF check: time_step_duration / dy  * abs(settling_speed) < 1
baseline params:

    const Field1D<float> column_density(5, 10.0f);
    const float settling_speed = 0.5f;
    const float precipitation_rate = 0.1f;
    const float dy = 10.0f;
    const float dt = 0.1f;
    
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
    column_density = [10,5,8]
    column_density = Field1D(5, -1)
    column_density = Field1D(1, -0.000001)
    column_density = Field1D(5, 0)
    column_density = Field1D(1, 0.000001)
    column_density = Field1D(5, 0.1)
    column_density = Field1D(5, 99999)
*/