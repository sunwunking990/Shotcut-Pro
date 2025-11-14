#pragma once

#include "widget_system.h"
#include "theme_system.h"
#include "math_types.h"
#include <memory>
#include <string>

namespace Shotcut::UI {

// Simple UI renderer for visual display (placeholder for now)
class UIRenderer {
private:
    // Rendering state
    std::shared_ptr<Theme> current_theme_;
    bool initialized_;
    
public:
    UIRenderer();
    ~UIRenderer();
    
    // Initialize renderer resources
    bool initialize();
    void cleanup();
    
    // Theme management
    void set_theme(std::shared_ptr<Theme> theme);
    const Theme& get_theme() const { return *current_theme_; }
    
    // Basic drawing primitives (console/text-based for now)
    void draw_rectangle(const glm::vec2& position, const glm::vec2& size, 
                      const glm::vec4& color, float border_radius = 0.0f);
    void draw_text(const glm::vec2& position, const std::string& text, 
                  const glm::vec4& color, float font_size = 16.0f);
    void draw_button(const glm::vec2& position, const glm::vec2& size,
                   const std::string& text, const glm::vec4& bg_color,
                   const glm::vec4& text_color, float border_radius = 4.0f);
    void draw_panel(const glm::vec2& position, const glm::vec2& size,
                  const glm::vec4& bg_color, float border_radius = 0.0f);
    
    // Widget rendering
    void render_widget(const std::shared_ptr<Widget>& widget);
    
    // Frame management
    void begin_frame();
    void end_frame();
    void present_frame();
    
private:
    // Drawing helpers (console-based for demo)
    void draw_console_representation();
};

} // namespace Shotcut::UI
