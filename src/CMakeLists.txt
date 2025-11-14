# Source directory CMakeLists.txt for ShotcutCPP

# Find required packages
find_package(Vulkan REQUIRED)

# Try to find GLFW (for windowing)
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW3 not found, will use platform windowing")
endif()

# Try to find PkgConfig (mainly for Linux)
find_package(PkgConfig QUIET)

# Find FFmpeg components if PkgConfig is available
if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG 
        libavcodec 
        libavformat 
        libavutil 
        libswscale
    )
endif()

# Platform-specific packages
if(PLATFORM_WINDOWS)
    find_package(DirectX12 QUIET)
elseif(PLATFORM_LINUX AND PkgConfig_FOUND)
    pkg_check_modules(VULKAN vulkan)
    find_package(X11 REQUIRED)
elseif(PLATFORM_MACOS)
    find_package(Metal REQUIRED)
    find_package(Cocoa REQUIRED)
endif()

# Add subdirectories
add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(rendering)
# Temporarily disable UI to focus on core functionality
add_subdirectory(ui)
add_subdirectory(video)
add_subdirectory(audio)
add_subdirectory(timeline)
add_subdirectory(effects)
add_subdirectory(utils)

# Simple Working Demo
add_executable(shotcut_simple_demo
    simple_working_demo.cpp
)

# Link simple demo with core libraries
target_link_libraries(shotcut_simple_demo
    PRIVATE
    shotcut_core
    shotcut_platform
    shotcut_video
    shotcut_audio
    shotcut_effects
    shotcut_utils
    ${CMAKE_THREAD_LIBS_INIT}
)

# Main executable
add_executable(ShotcutCPP
    main.cpp
)

# Link libraries
target_link_libraries(ShotcutCPP
    PRIVATE
    # Core libraries
    shotcut_core
    shotcut_platform
    shotcut_rendering
    # Temporarily disable UI
    shotcut_ui
    shotcut_video
    shotcut_audio
    shotcut_timeline
    shotcut_effects
    shotcut_utils
    
    # External libraries (conditionally added)
    Vulkan::Vulkan
)

# Conditionally link optional libraries
if(glfw3_FOUND)
    target_link_libraries(ShotcutCPP PRIVATE glfw)
endif()

if(FFMPEG_FOUND)
    target_link_libraries(ShotcutCPP PRIVATE ${FFMPEG_LIBRARIES})
endif()

# Include directories
target_include_directories(ShotcutCPP
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${VULKAN_INCLUDE_DIRS}
)

if(FFMPEG_FOUND)
    target_include_directories(ShotcutCPP PRIVATE ${FFMPEG_INCLUDE_DIRS})
endif()

# Apply security hardening
apply_default_hardening(ShotcutCPP)
configure_security(ShotcutCPP)

# Platform-specific linking
if(PLATFORM_WINDOWS)
    if(DirectX12_FOUND)
        target_link_libraries(ShotcutCPP PRIVATE DirectX12::DirectX12)
    endif()
elseif(PLATFORM_LINUX)
    if(X11_FOUND)
        target_link_libraries(ShotcutCPP PRIVATE ${X11_LIBRARIES})
    endif()
    if(VULKAN_LIBRARIES)
        target_link_libraries(ShotcutCPP PRIVATE ${VULKAN_LIBRARIES})
    endif()
elseif(PLATFORM_MACOS)
    target_link_libraries(ShotcutCPP PRIVATE ${Metal_LIBRARIES} ${Cocoa_LIBRARIES})
endif()

# Compiler-specific settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ShotcutCPP PRIVATE -Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(ShotcutCPP PRIVATE /W4 /WX)
endif()
