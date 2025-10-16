cmake_minimum_required(VERSION 3.18)

project(SnowSim VERSION 0.1.0 LANGUAGES C CXX)

# toggles to swap between CPU/GPU backend and to turn on/off build tests
option(ENABLE_CUDA "Enable CUDA backend" OFF)
option(SNOWSIM_ENABLE_TESTS "Build SnowSim unit tests" ON)

if(ENABLE_CUDA)
  enable_language(CUDA)
endif()

add_library(snow_sim STATIC
  src/cpu_backend.cpp
)

target_include_directories(snow_sim PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/external/include
)
target_compile_features(snow_sim PUBLIC cxx_std_17)

if(ENABLE_CUDA AND CMAKE_CUDA_COMPILER)
  target_sources(snow_sim PRIVATE src/cuda_backend.cu)
  target_compile_definitions(snow_sim PUBLIC SNOWSIM_HAS_CUDA=1)
  set_target_properties(snow_sim PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
else()
  target_compile_definitions(snow_sim PUBLIC SNOWSIM_HAS_CUDA=0)
endif()

find_package(OpenGL REQUIRED)

add_executable(snow_sim_app
  src/main.cpp
  src/my_helper.cpp
  viz/renderer.cpp
  viz/gl.c
  viz/shader_program.cpp
  viz/cube_mesh.cpp
  viz/arrow_layer.cpp
  viz/grid_mesh.cpp
  viz/camera.cpp
)

target_include_directories(snow_sim_app PRIVATE
  ${CMAKE_SOURCE_DIR}/viz
  ${CMAKE_SOURCE_DIR}/external/include
)

target_link_directories(snow_sim_app PRIVATE
  ${CMAKE_SOURCE_DIR}/external/lib
)

target_link_libraries(snow_sim_app PRIVATE
  snow_sim
  OpenGL::GL
  ${CMAKE_SOURCE_DIR}/external/lib/glfw3.lib
  gdi32.lib
  shell32.lib
  user32.lib
  advapi32.lib
  kernel32.lib
  ole32.lib
  uuid.lib
  winmm.lib
)
# TODO: replace the prebuilt GLFW binary with one compiled against the same MSVC runtime to silence the LNK4098 warning.
# TODO: replace prebuilt GLFW binary with a version built using the same MSVC runtime to remove LNK4098 warning.

set(GLFW_DLL ${CMAKE_SOURCE_DIR}/external/glfw3.dll)
if(EXISTS ${GLFW_DLL})
  add_custom_command(TARGET snow_sim_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GLFW_DLL}
            $<TARGET_FILE_DIR:snow_sim_app>)
endif()

add_custom_command(TARGET snow_sim_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/resources
          $<TARGET_FILE_DIR:snow_sim_app>/resources)

set_property(TARGET snow_sim_app PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# test config enable tests with SNOWSIM_ENABLE_TESTS
if(SNOWSIM_ENABLE_TESTS)
  enable_testing()

  add_executable(snow_sim_unit_tests
    tests/unit/test_tests.cpp
    tests/unit/catch_amalgamated.cpp
  )

  target_include_directories(snow_sim_unit_tests PRIVATE
    include
    tests/unit
    tests/integration
    ${CMAKE_SOURCE_DIR}/external/include
  )

  target_link_libraries(snow_sim_unit_tests PRIVATE snow_sim)
  target_compile_features(snow_sim_unit_tests PRIVATE cxx_std_17)

  add_test(NAME SnowSimUnitTests COMMAND snow_sim_unit_tests)
endif()
