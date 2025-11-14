# Utils module CMakeLists.txt

# Create utils library
add_library(shotcut_utils
    utils.cpp
)

# Include directories
target_include_directories(shotcut_utils
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Apply security hardening
apply_default_hardening(shotcut_utils)
configure_security(shotcut_utils)

# Link standard library features
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(shotcut_utils PUBLIC pthread)
endif()
