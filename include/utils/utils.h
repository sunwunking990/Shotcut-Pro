#pragma once

#include <string>
#include <memory>

namespace Shotcut::Utils {

// Format bytes to human readable string
std::string format_bytes(size_t bytes);

// Basic utility initialization
bool initialize();

// Basic utility cleanup
void shutdown();

} // namespace Shotcut::Utils
