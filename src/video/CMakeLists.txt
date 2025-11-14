# Video module CMakeLists.txt
# Professional video processing, export, and effects pipeline

# Find required packages
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG 
        libavcodec 
        libavformat 
        libavutil 
        libswscale 
        libavfilter
    )
endif()

# Find Vulkan for effects processing
find_package(Vulkan QUIET)

# Find NVIDIA Video SDK (optional)
find_path(NVIDIA_VIDEO_SDK_ROOT
    PATHS ${PROJECT_SOURCE_DIR}/../external/nvidia_video_sdk
          ${PROJECT_SOURCE_DIR}/external_dependencies/nvidia_video_sdk
          /opt/nvidia/video_sdk
          C:/Program Files/NVIDIA Corporation/NVIDIA Video SDK
    DOC "NVIDIA Video SDK root directory"
)

if(NVIDIA_VIDEO_SDK_ROOT)
    set(NVIDIA_VIDEO_SDK_INCLUDE_DIRS ${NVIDIA_VIDEO_SDK_ROOT}/include)
    set(NVIDIA_VIDEO_SDK_LIBRARIES_DIR ${NVIDIA_VIDEO_SDK_ROOT}/lib/${CMAKE_LIBRARY_ARCHITECTURE})
    
    # Set library names
    if(WIN32)
        set(NVIDIA_VIDEO_SDK_LIBRARIES 
            nvidia-encode
            nvidia-decode
            cuda
            ${NVIDIA_VIDEO_SDK_LIBRARIES_DIR}/nvidia-encode.lib
            ${NVIDIA_VIDEO_SDK_LIBRARIES_DIR}/nvidia-decode.lib
            ${NVIDIA_VIDEO_SDK_LIBRARIES_DIR}/cuda.lib
        )
    else()
        set(NVIDIA_VIDEO_SDK_LIBRARIES 
            nvidia-encode
            nvidia-decode
            cuda
        )
    endif()
    
    set(NVIDIA_VIDEO_SDK_FOUND TRUE)
else()
    set(NVIDIA_VIDEO_SDK_FOUND FALSE)
    message(STATUS "NVIDIA Video SDK not found - export acceleration will be disabled")
endif()

# Note: external_dependencies directory not created yet
# add_subdirectory(external_dependencies)

# Main video library
add_library(shotcut_video
    video.cpp
    ffmpeg_wrapper.cpp
    frame_buffer.cpp
    nvidia_video_sdk.cpp
    export_system.cpp
    video_demo.cpp
    export_demo.cpp
)

# Link libraries for shotcut_video
target_link_libraries(shotcut_video
    PUBLIC
    shotcut_core
    shotcut_ui
    shotcut_platform
    shotcut_utils
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(shotcut_video
        PUBLIC
            dxgi
            d3d11
            mf
            mfplat
            mfuuid
    )
endif()

# Optional dependencies
if(FFMPEG_FOUND)
    target_link_libraries(shotcut_video PUBLIC ${FFMPEG_LIBRARIES})
    target_include_directories(shotcut_video PUBLIC ${FFMPEG_INCLUDE_DIRS})
    target_compile_definitions(shotcut_video PUBLIC ENABLE_FFMPEG)
endif()

if(Vulkan_FOUND)
    target_link_libraries(shotcut_video PUBLIC Vulkan::Vulkan)
    target_include_directories(shotcut_video PUBLIC ${Vulkan_INCLUDE_DIRS})
    target_compile_definitions(shotcut_video PUBLIC ENABLE_VULKAN)
endif()

if(NVIDIA_VIDEO_SDK_FOUND)
    target_link_libraries(shotcut_video PUBLIC ${NVIDIA_VIDEO_SDK_LIBRARIES})
    target_include_directories(shotcut_video PUBLIC ${NVIDIA_VIDEO_SDK_INCLUDE_DIRS})
    target_compile_definitions(shotcut_video PUBLIC ENABLE_NVIDIA_VIDEO_SDK)
endif()

# Include directories
target_include_directories(shotcut_video
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(shotcut_video PRIVATE
        -Wall -Wextra -Wpedantic
        -fno-strict-aliasing
        -Wno-unused-parameter
    )
    # Enable C++20/23 features
    target_compile_options(shotcut_video PRIVATE
        -std=c++20
        -fcoroutines-ts
    )
    # Security hardening
    target_compile_options(shotcut_video PRIVATE
        -fstack-protector-strong
        -fcf-protection=full
        -D_FORTIFY_SOURCE=3
        -fPIE
    )
    # Optimizations
    target_compile_options(shotcut_video PRIVATE
        -O3 -march=native -mtune=native
        -flto
        -ffast-math
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(shotcut_video PRIVATE
        /W4 /WX
        /std:c++20
        /await:strict
        /permissive-
    )
    # Security hardening
    target_compile_options(shotcut_video PRIVATE
        /GS
        /guard:cf
        /DYNAMICBASE
        /sdl
    )
    # Optimizations
    target_compile_options(shotcut_video PRIVATE
        /O2 /Ob2 /Oi /Ot
        /GL
    )
endif()

# Apply security hardening
apply_default_hardening(shotcut_video)
configure_security(shotcut_video)

# Set properties
set_target_properties(shotcut_video PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    POSITION_INDEPENDENT_CODE ON
)

# Installation
install(TARGETS shotcut_video
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Video module tests (tests directory not created yet)
# enable_testing()
# add_subdirectory(tests)

# Package configuration
set(CPACK_COMPONENT shotcut_video
    DISPLAY_NAME "Shotcut Video Processing"
    DESCRIPTION "Professional video processing and export system"
    VERSION ${PROJECT_VERSION}
)
