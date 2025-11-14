# Timeline Module CMakeLists.txt
# Entity Component System for Professional Video Editing

set(TIMELINE_SOURCES
    entity.cpp
)

set(TIMELINE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/timeline/entity.h
    ${CMAKE_SOURCE_DIR}/include/timeline/components.h
    ${CMAKE_SOURCE_DIR}/include/timeline/systems.h
)

# Create timeline library
add_library(shotcut_timeline STATIC ${TIMELINE_SOURCES})

target_include_directories(shotcut_timeline
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include/timeline
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link with required libraries
target_link_libraries(shotcut_timeline
    PRIVATE
        shotcut_ui
        shotcut_video
        shotcut_core
)

# Set C++ standard with explicit flags
if(MSVC)
    target_compile_options(shotcut_timeline PRIVATE /std:c++20 /permissive-)
else()
    set_property(TARGET shotcut_timeline PROPERTY
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
    )
endif()

# Apply security flags
target_compile_options(shotcut_timeline PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${SECURITY_COMPILE_FLAGS}>
)

# Timeline demo executable
add_executable(timeline_demo timeline_demo.cpp)

target_include_directories(timeline_demo
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/timeline
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(timeline_demo
    PRIVATE
        shotcut_timeline
        shotcut_ui
        shotcut_video
        shotcut_core
)

# Apply security flags to demo
target_compile_options(timeline_demo PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${SECURITY_COMPILE_FLAGS}>
)

# Set properties for IDE organization
set_target_properties(shotcut_timeline PROPERTIES
    FOLDER "Timeline"
    PROJECT_LABEL "Timeline ECS"
)

set_target_properties(timeline_demo PROPERTIES
    FOLDER "Timeline/Demos"
    PROJECT_LABEL "Timeline Demo"
)

# Add to global targets list
set(GLOBAL_TARGETS ${GLOBAL_TARGETS} shotcut_timeline timeline_demo CACHE INTERNAL "")
