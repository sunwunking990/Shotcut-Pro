# Core module CMakeLists.txt

# Create core library
add_library(shotcut_core
    application.cpp
    application.h
)

# Include directories
target_include_directories(shotcut_core
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/../ui
)

# Apply security hardening
apply_default_hardening(shotcut_core)
configure_security(shotcut_core)

# Link Vulkan for headers
target_link_libraries(shotcut_core PUBLIC Vulkan::Vulkan)
target_include_directories(shotcut_core PRIVATE ${VULKAN_INCLUDE_DIRS})

# Link standard library features
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_libraries(shotcut_core PUBLIC pthread)
endif()
