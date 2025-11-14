# Tests module CMakeLists.txt

# Enable testing
enable_testing()

# Find required packages for testing
find_package(Catch2 3 QUIET)

if(NOT Catch2_FOUND)
    # Use FetchContent to get Catch2 if not found
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Add test executables
# TODO: Add test files as they are created

# Include directories
target_include_directories(shotcut_tests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link with main libraries for testing
target_link_libraries(shotcut_tests
    PRIVATE
    shotcut_core
    shotcut_rendering
    shotcut_platform
    shotcut_ui
    shotcut_video
    shotcut_audio
    shotcut_utils
    Catch2::Catch2WithMain
)

# Apply security hardening to tests
apply_default_hardening(shotcut_tests)
configure_security(shotcut_tests)

# Add tests to CTest
add_test(NAME shotcut_unit_tests COMMAND shotcut_tests)
