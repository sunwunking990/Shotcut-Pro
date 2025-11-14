# UI module CMakeLists.txt

# Create UI library
add_library(shotcut_ui
    layout_engine.h
    widget_system.h
    theme_system.h
    input_manager.h
    timeline_widget.h
    ui_renderer.h
    math_types.h
    vulkan_ui_renderer.h
    layout_engine.cpp
    widget_system.cpp
    theme_system.cpp
    timeline_widget.cpp
    ui_renderer.cpp
    input_manager.cpp
    vulkan_ui_renderer.cpp
)

# Include directories
target_include_directories(shotcut_ui
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link dependencies
target_link_libraries(shotcut_ui
    PUBLIC
    shotcut_core
    shotcut_rendering
    Vulkan::Vulkan
    glfw
)
# TODO: Add glm back when available

# Apply security hardening
apply_default_hardening(shotcut_ui)
configure_security(shotcut_ui)
