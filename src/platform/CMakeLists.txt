# Platform module CMakeLists.txt

# Create platform library
add_library(shotcut_platform
    platform.cpp
)

# Include directories
target_include_directories(shotcut_platform
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Apply security hardening
apply_default_hardening(shotcut_platform)
configure_security(shotcut_platform)
