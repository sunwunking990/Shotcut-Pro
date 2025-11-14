#include "theme_system.h"
#include <fstream>
#include <sstream>
#include <memory>

namespace Shotcut::UI {

// Built-in theme implementations
std::unique_ptr<Theme> Theme::create_dark_theme() {
    auto theme = std::make_unique<Theme>("dark");
    
    // Background colors
    theme->set_property("background", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(0.15f, 0.15f, 0.15f, 1.0f)
    });
    
    theme->set_property("panel_background", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)
    });
    
    // Text colors
    theme->set_property("text_color", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)
    });
    
    theme->set_property("text_secondary", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
    });
    
    // Accent colors
    theme->set_property("accent", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.2f, 0.6f, 1.0f, 1.0f)
    });
    
    theme->set_property("accent_hover", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.3f, 0.7f, 1.0f, 1.0f)
    });
    
    // Border colors
    theme->set_property("border", StyleProperty{
        StylePropertyType::BorderColor,
        glm::vec4(0.3f, 0.3f, 0.3f, 1.0f)
    });
    
    theme->set_property("border_light", StyleProperty{
        StylePropertyType::BorderColor,
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f)
    });
    
    // Typography
    theme->set_property("font_size", StyleProperty{
        StylePropertyType::FontSize,
        14.0f
    });
    
    theme->set_property("font_family", StyleProperty{
        StylePropertyType::FontFamily,
        std::string("Inter, system-ui, sans-serif")
    });
    
    // Spacing
    theme->set_property("padding", StyleProperty{
        StylePropertyType::Padding,
        glm::vec4(8.0f, 8.0f, 8.0f, 8.0f) // top, right, bottom, left
    });
    
    theme->set_property("margin", StyleProperty{
        StylePropertyType::Margin,
        glm::vec4(4.0f, 4.0f, 4.0f, 4.0f)
    });
    
    theme->set_property("border_radius", StyleProperty{
        StylePropertyType::BorderRadius,
        4.0f
    });
    
    return theme;
}

std::unique_ptr<Theme> Theme::create_light_theme() {
    auto theme = std::make_unique<Theme>("light");
    
    // Background colors
    theme->set_property("background", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)
    });
    
    theme->set_property("panel_background", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(0.95f, 0.95f, 0.95f, 1.0f)
    });
    
    // Text colors
    theme->set_property("text_color", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.1f, 0.1f, 0.1f, 1.0f)
    });
    
    theme->set_property("text_secondary", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.4f, 0.4f, 0.4f, 1.0f)
    });
    
    // Accent colors
    theme->set_property("accent", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.0f, 0.4f, 0.8f, 1.0f)
    });
    
    theme->set_property("accent_hover", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.1f, 0.5f, 0.9f, 1.0f)
    });
    
    // Border colors
    theme->set_property("border", StyleProperty{
        StylePropertyType::BorderColor,
        glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)
    });
    
    theme->set_property("border_light", StyleProperty{
        StylePropertyType::BorderColor,
        glm::vec4(0.3f, 0.3f, 0.3f, 1.0f)
    });
    
    // Typography
    theme->set_property("font_size", StyleProperty{
        StylePropertyType::FontSize,
        14.0f
    });
    
    theme->set_property("font_family", StyleProperty{
        StylePropertyType::FontFamily,
        std::string("Inter, system-ui, sans-serif")
    });
    
    // Spacing
    theme->set_property("padding", StyleProperty{
        StylePropertyType::Padding,
        glm::vec4(8.0f, 8.0f, 8.0f, 8.0f)
    });
    
    theme->set_property("margin", StyleProperty{
        StylePropertyType::Margin,
        glm::vec4(4.0f, 4.0f, 4.0f, 4.0f)
    });
    
    theme->set_property("border_radius", StyleProperty{
        StylePropertyType::BorderRadius,
        4.0f
    });
    
    return theme;
}

std::unique_ptr<Theme> Theme::create_professional_theme() {
    auto theme = std::make_unique<Theme>("professional");
    
    // Professional dark theme similar to industry standards
    // Background colors
    theme->set_property("background", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(0.12f, 0.12f, 0.13f, 1.0f)
    });
    
    theme->set_property("panel_background", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(0.18f, 0.18f, 0.20f, 1.0f)
    });
    
    theme->set_property("surface", StyleProperty{
        StylePropertyType::BackgroundColor,
        glm::vec4(0.22f, 0.22f, 0.25f, 1.0f)
    });
    
    // Text colors
    theme->set_property("text_color", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.97f, 0.97f, 0.97f, 1.0f)
    });
    
    theme->set_property("text_secondary", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.65f, 0.65f, 0.65f, 1.0f)
    });
    
    theme->set_property("text_muted", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.45f, 0.45f, 0.45f, 1.0f)
    });
    
    // Accent colors - professional blue
    theme->set_property("accent", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.0f, 0.48f, 1.0f, 1.0f)
    });
    
    theme->set_property("accent_hover", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.2f, 0.6f, 1.0f, 1.0f)
    });
    
    theme->set_property("accent_active", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.0f, 0.35f, 0.8f, 1.0f)
    });
    
    // Status colors
    theme->set_property("success", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.0f, 0.8f, 0.4f, 1.0f)
    });
    
    theme->set_property("warning", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(1.0f, 0.6f, 0.0f, 1.0f)
    });
    
    theme->set_property("error", StyleProperty{
        StylePropertyType::Color,
        glm::vec4(0.9f, 0.2f, 0.2f, 1.0f)
    });
    
    // Border colors
    theme->set_property("border", StyleProperty{
        StylePropertyType::BorderColor,
        glm::vec4(0.3f, 0.3f, 0.35f, 1.0f)
    });
    
    theme->set_property("border_focus", StyleProperty{
        StylePropertyType::BorderColor,
        glm::vec4(0.0f, 0.48f, 1.0f, 1.0f)
    });
    
    // Typography
    theme->set_property("font_size", StyleProperty{
        StylePropertyType::FontSize,
        13.0f
    });
    
    theme->set_property("font_family", StyleProperty{
        StylePropertyType::FontFamily,
        std::string("SF Pro, -apple-system, BlinkMacSystemFont, Segoe UI, Roboto, sans-serif")
    });
    
    theme->set_property("font_size_small", StyleProperty{
        StylePropertyType::FontSize,
        11.0f
    });
    
    theme->set_property("font_size_large", StyleProperty{
        StylePropertyType::FontSize,
        16.0f
    });
    
    // Spacing
    theme->set_property("padding_small", StyleProperty{
        StylePropertyType::Padding,
        glm::vec4(4.0f, 4.0f, 4.0f, 4.0f)
    });
    
    theme->set_property("padding", StyleProperty{
        StylePropertyType::Padding,
        glm::vec4(8.0f, 8.0f, 8.0f, 8.0f)
    });
    
    theme->set_property("padding_large", StyleProperty{
        StylePropertyType::Padding,
        glm::vec4(16.0f, 16.0f, 16.0f, 16.0f)
    });
    
    theme->set_property("margin", StyleProperty{
        StylePropertyType::Margin,
        glm::vec4(6.0f, 6.0f, 6.0f, 6.0f)
    });
    
    theme->set_property("border_radius_small", StyleProperty{
        StylePropertyType::BorderRadius,
        2.0f
    });
    
    theme->set_property("border_radius", StyleProperty{
        StylePropertyType::BorderRadius,
        6.0f
    });
    
    theme->set_property("border_radius_large", StyleProperty{
        StylePropertyType::BorderRadius,
        8.0f
    });
    
    // Shadow
    theme->set_property("shadow", StyleProperty{
        StylePropertyType::Shadow,
        std::vector<glm::vec4>{
            glm::vec4(0.0f, 2.0f, 8.0f, 0.15f), // offset_x, offset_y, blur, alpha
            glm::vec4(0.0f, 1.0f, 3.0f, 0.1f)
        }
    });
    
    return theme;
}

} // namespace Shotcut::UI
