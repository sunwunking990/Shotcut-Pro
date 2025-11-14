# External dependencies module CMakeLists.txt

# Include FetchContent for external dependencies
include(FetchContent)

# Skip GLFW download for now to avoid git permission issues
# We'll use platform windowing instead
if(NOT TARGET glfw)
    message(STATUS "Skipping GLFW download, using platform windowing")
endif()

# Fetch Vulkan headers if needed
if(NOT Vulkan_FOUND)
    FetchContent_Declare(
        VulkanHeaders
        GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG v1.3.275
    )
    FetchContent_MakeAvailable(VulkanHeaders)
endif()

# GLM will be added later - for now using basic math types
# TODO: Add GLM back when git permissions are resolved

# Fetch other dependencies as needed
# TODO: Add other external dependencies (FreeType, HarfBuzz, etc.)
