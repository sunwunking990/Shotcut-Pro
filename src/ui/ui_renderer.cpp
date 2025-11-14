#include "ui_renderer.h"
#include <iostream>

namespace Shotcut::UI {

UIRenderer::UIRenderer() : initialized_(false) {
}

UIRenderer::~UIRenderer() {
    cleanup();
}

bool UIRenderer::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Create default theme
    current_theme_ = std::make_shared<Theme>();
    
    initialized_ = true;
    return true;
}

void UIRenderer::cleanup() {
    if (!initialized_) {
        return;
    }
    
    current_theme_.reset();
    initialized_ = false;
}

void UIRenderer::set_theme(std::shared_ptr<Theme> theme) {
    if (theme) {
        current_theme_ = theme;
    }
}

void UIRenderer::draw_rectangle(const glm::vec2& position, const glm::vec2& size, 
                               const glm::vec4& color, float border_radius) {
    // Placeholder for console output
    std::cout << "[Rectangle] Pos(" << position.x << "," << position.y 
              << ") Size(" << size.x << "x" << size.y << ")" << std::endl;
}

void UIRenderer::draw_text(const glm::vec2& position, const std::string& text, 
                          const glm::vec4& color, float font_size) {
    std::cout << "[Text] \"" << text << "\" at (" << position.x << "," << position.y << ")" << std::endl;
}

void UIRenderer::draw_button(const glm::vec2& position, const glm::vec2& size,
                            const std::string& text, const glm::vec4& bg_color,
                            const glm::vec4& text_color, float border_radius) {
    draw_rectangle(position, size, bg_color, border_radius);
    draw_text({position.x + size.x * 0.5f, position.y + size.y * 0.5f}, 
              text, text_color, 14.0f);
}

void UIRenderer::draw_panel(const glm::vec2& position, const glm::vec2& size,
                           const glm::vec4& bg_color, float border_radius) {
    draw_rectangle(position, size, bg_color, border_radius);
}

void UIRenderer::render_widget(const std::shared_ptr<Widget>& widget) {
    if (!widget) {
        return;
    }
    
    // Placeholder implementation
    std::cout << "Rendering widget" << std::endl;
}

void UIRenderer::begin_frame() {
    std::cout << "UI Frame Begin" << std::endl;
}

void UIRenderer::end_frame() {
    std::cout << "UI Frame End" << std::endl;
}

void UIRenderer::present_frame() {
    std::cout << "Frame Presented" << std::endl;
}

} // namespace Shotcut::UI
