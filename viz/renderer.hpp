#pragma once

#include <cstdint>

namespace snow {
namespace viz {

bool initialize(std::int32_t width, std::int32_t height, const char* title);
void shutdown();

void poll_events();
bool should_close();

void begin_frame();
void end_frame();

} // namespace viz
} // namespace snow

