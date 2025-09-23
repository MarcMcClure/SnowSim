#pragma once

#include <cstdint>

namespace snow {
namespace viz {

bool initialize(std::int32_t width, std::int32_t height, const char* title);
void shutdown();

void poll_events();
void process_input();
bool should_close();

void begin_frame();
void end_frame();

bool visualizer_is_closed();

} // namespace viz
} // namespace snow

