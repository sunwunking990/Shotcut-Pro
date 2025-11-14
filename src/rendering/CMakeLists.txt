# Rendering module CMakeLists.txt

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 QUIET)

# Create rendering library
add_library(shotcut_rendering
    vulkan_context.cpp
    vulkan_context.h
    vulkan_swapchain.cpp
    vulkan_swapchain.h
    vulkan_graphics_pipeline.cpp
    vulkan_graphics_pipeline.h
    vulkan_command_buffer.cpp
    vulkan_command_buffer.h
)

# Link libraries
target_link_libraries(shotcut_rendering
    PUBLIC
    Vulkan::Vulkan
)

if(glfw3_FOUND)
    target_link_libraries(shotcut_rendering PUBLIC glfw)
    target_compile_definitions(shotcut_rendering PUBLIC HAVE_GLFW)
endif()

# Include directories
target_include_directories(shotcut_rendering
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${Vulkan_INCLUDE_DIRS}
)

# Apply security hardening
apply_default_hardening(shotcut_rendering)
configure_security(shotcut_rendering)

# Compiler definitions
target_compile_definitions(shotcut_rendering
    PUBLIC
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)

# Platform-specific settings
if(PLATFORM_WINDOWS)
    target_compile_definitions(shotcut_rendering PUBLIC VK_USE_PLATFORM_WIN32_KHR)
elseif(PLATFORM_LINUX)
    target_compile_definitions(shotcut_rendering PUBLIC VK_USE_PLATFORM_XCB_KHR)
elseif(PLATFORM_MACOS)
    target_compile_definitions(shotcut_rendering PUBLIC VK_USE_PLATFORM_METAL_EXT)
endif()
