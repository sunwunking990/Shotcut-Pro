# Effects Module CMakeLists.txt
# GPU Effects System for Professional Video Editing

set(EFFECTS_SOURCES
    effect_system.cpp
)

set(EFFECTS_HEADERS
    ${CMAKE_SOURCE_DIR}/include/effects/effect_system.h
)

# Create effects library
add_library(shotcut_effects STATIC ${EFFECTS_SOURCES})

target_include_directories(shotcut_effects
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include/effects
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link with required libraries
target_link_libraries(shotcut_effects
    PRIVATE
        shotcut_ui
        shotcut_video
        shotcut_timeline
        shotcut_core
)

# Find and link Vulkan
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    target_link_libraries(shotcut_effects PRIVATE Vulkan::Vulkan)
    target_compile_definitions(shotcut_effects PRIVATE VULKAN_ENABLED)
else()
    message(WARNING "Vulkan not found, effects system will be disabled")
    target_compile_definitions(shotcut_effects PRIVATE VULKAN_DISABLED)
endif()

# Set C++ standard with explicit flags
if(MSVC)
    target_compile_options(shotcut_effects PRIVATE /std:c++20 /permissive-)
else()
    set_property(TARGET shotcut_effects PROPERTY
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endif()

# Apply security flags
target_compile_options(shotcut_effects PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${SECURITY_COMPILE_FLAGS}>
)

# Effects demo executable
add_executable(effects_demo effects_demo.cpp)

target_include_directories(effects_demo
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/effects
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(effects_demo
    PRIVATE
        shotcut_effects
        shotcut_ui
        shotcut_video
        shotcut_timeline
        shotcut_core
)

# Apply security flags to demo
target_compile_options(effects_demo PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${SECURITY_COMPILE_FLAGS}>
)

# Set properties for IDE organization
set_target_properties(shotcut_effects PROPERTIES
    FOLDER "Effects"
    PROJECT_LABEL "GPU Effects System"
)

set_target_properties(effects_demo PROPERTIES
    FOLDER "Effects/Demos"
    PROJECT_LABEL "Effects Demo"
)

# Add to global targets list
set(GLOBAL_TARGETS ${GLOBAL_TARGETS} shotcut_effects effects_demo CACHE INTERNAL "")
