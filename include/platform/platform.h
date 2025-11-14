#pragma once

#include <string>

namespace shotcut {
namespace platform {

struct PlatformInfo {
    std::string name;
    std::string version;
};

// Platform initialization and cleanup
bool initialize();
void shutdown();

// Get platform information
PlatformInfo get_platform_info();

} // namespace platform
} // namespace shotcut
