// Utility functions for ShotcutCPP
// Placeholder implementation for now

#include "utils/utils.h"
#include <string>
#include <sstream>

namespace Shotcut::Utils {

// Format bytes to human readable string
std::string format_bytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    
    std::ostringstream oss;
    oss << size << " " << units[unit];
    return oss.str();
}

// Basic utility initialization
bool initialize() {
    return true;
}

// Basic utility cleanup
void shutdown() {
    // Utility-specific cleanup
}

} // namespace Shotcut::Utils
