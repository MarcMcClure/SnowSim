#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

namespace test_support {

// Captures everything written to std::cerr while the callable runs and returns it.
inline std::string capture_stderr(std::function<void()> fn) {
    std::ostringstream oss;
    auto* const old_buf = std::cerr.rdbuf(oss.rdbuf());
    try {
        fn();
    } catch (...) {
        std::cerr.rdbuf(old_buf);
        throw;
    }
    std::cerr.rdbuf(old_buf);
    return oss.str();
}

} // namespace test_support
