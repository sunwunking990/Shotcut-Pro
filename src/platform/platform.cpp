// Platform-specific implementation for ShotcutCPP
// This file provides basic platform functionality

#include "platform/platform.h"

namespace shotcut {
namespace platform {

// Basic platform initialization
bool initialize() {
    return true;
}

// Basic platform cleanup
void shutdown() {
    // Platform-specific cleanup
}

// Get platform information
PlatformInfo get_platform_info() {
    PlatformInfo info;
#ifdef PLATFORM_WINDOWS
    info.name = "Windows";
    info.version = "10+";
#elif defined(PLATFORM_LINUX)
    info.name = "Linux";
    info.version = "Unknown";
#elif defined(PLATFORM_MACOS)
    info.name = "macOS";
    info.version = "Unknown";
#endif
    return info;
}

} // namespace platform
} // namespace shotcut
