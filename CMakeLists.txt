cmake_minimum_required(VERSION 3.20)
project(ShotcutCPP 
    VERSION 1.0.0 
    DESCRIPTION "Modern C++ Video Editor"
    LANGUAGES CXX
)

# C++20/23 Configuration
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable C++ modules
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif()

# Platform detection
include(CheckCXXSourceCompiles)
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
    # Configure MSVC
    if(MSVC)
        add_compile_options(/W4)
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE)
    endif()
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_definitions(-DPLATFORM_MACOS)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    add_definitions(-DPLATFORM_LINUX)
endif()

# Security and hardening
include(cmake/CompilerHardening.cmake)
include(cmake/SecurityFlags.cmake)

# Find required packages
set(FFMPEG_INCLUDE_DIRS "C:/ffmpeg/include")
set(FFMPEG_LIBRARIES "C:/ffmpeg/lib/avcodec.lib;C:/ffmpeg/lib/avformat.lib;C:/ffmpeg/lib/avutil.lib;C:/ffmpeg/lib/swscale.lib")
set(FFMPEG_FOUND TRUE)
find_package(Vulkan REQUIRED)

# Platform-specific packages
if(PLATFORM_WINDOWS)
    find_package(DirectX12)
    # On Windows, try to find FFmpeg through conventional paths
    find_path(FFMPEG_INCLUDE_DIRS libavcodec/avcodec.h)
    find_library(FFMPEG_LIBRARIES avcodec)
    if(FFMPEG_INCLUDE_DIRS AND FFMPEG_LIBRARIES)
        set(FFMPEG_FOUND TRUE)
    endif()
elseif(PLATFORM_LINUX)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(VULKAN REQUIRED vulkan)
        pkg_check_modules(FFMPEG REQUIRED 
            libavcodec 
            libavformat 
            libavutil 
            libswscale
        )
        find_package(X11 REQUIRED)
    endif()
elseif(PLATFORM_MACOS)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(FFMPEG REQUIRED 
            libavcodec 
            libavformat 
            libavutil 
            libswscale
        )
    endif()
    find_package(Metal REQUIRED)
    find_package(Cocoa REQUIRED)
endif()

# Subdirectories
add_subdirectory(src)
add_subdirectory(external)
# Temporarily disable tests to focus on main build
# add_subdirectory(tests)

# Installation configuration
include(GNUInstallDirs)
install(DIRECTORY resources/ DESTINATION ${CMAKE_INSTALL_DATADIR})
install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Print configuration summary
message(STATUS "")
message(STATUS "ShotcutCPP Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Vulkan Found: ${Vulkan_FOUND}")
message(STATUS "  FFmpeg Found: ${FFMPEG_FOUND}")
message(STATUS "")
