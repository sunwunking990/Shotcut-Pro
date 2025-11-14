# Audio module CMakeLists.txt

# Create audio library
add_library(shotcut_audio
    audio.cpp
)

# Include directories
target_include_directories(shotcut_audio
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Apply security hardening
apply_default_hardening(shotcut_audio)
configure_security(shotcut_audio)

# Link audio libraries (platform-specific)
if(PLATFORM_WINDOWS)
    # TODO: Add Windows audio libraries
elseif(PLATFORM_LINUX)
    # TODO: Add Linux audio libraries (ALSA, PulseAudio)
elseif(PLATFORM_MACOS)
    # TODO: Add macOS audio libraries (CoreAudio)
endif()
