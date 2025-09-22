cmake_minimum_required(VERSION 3.18)

project(SnowSim VERSION 0.1.0 LANGUAGES CXX)

option(ENABLE_CUDA "Enable CUDA backend" OFF)

if(ENABLE_CUDA)
  enable_language(CUDA)
endif()

add_library(snow_sim STATIC
  src/cpu_backend.cpp
)

target_include_directories(snow_sim PUBLIC include)
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
  viz/renderer.cpp
)

target_include_directories(snow_sim_app PRIVATE
  ${CMAKE_SOURCE_DIR}/viz
  ${CMAKE_SOURCE_DIR}/external/include
)

target_link_directories(snow_sim_app PRIVATE ${CMAKE_SOURCE_DIR}/external/lib)

target_link_libraries(snow_sim_app PRIVATE
  snow_sim
  OpenGL::GL
  ${CMAKE_SOURCE_DIR}/external/lib/glfw3.lib
  ${CMAKE_SOURCE_DIR}/external/lib/glew32.lib
  gdi32.lib
  shell32.lib
  user32.lib
  advapi32.lib
  kernel32.lib
  ole32.lib
  uuid.lib
  winmm.lib
)

add_custom_command(TARGET snow_sim_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/external/glew32.dll
          $<TARGET_FILE_DIR:snow_sim_app>)

set_property(TARGET snow_sim_app PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
