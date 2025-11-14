#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>
#include <optional>
#include <glm/glm.hpp>

namespace Shotcut::UI {

// Forward declarations
class Widget;

// Style property values using modern C++17/20 features
using StyleValue = std::variant<
    bool, int32_t, float, std::string,
    glm::vec2, glm::vec3, glm::vec4,
    std::vector<float>, std::vector<glm::vec4>
>;

enum class StylePropertyType : uint8_t {
    Color, BackgroundColor, BorderColor,
    Width, Height, MinWidth, MaxWidth, MinHeight, MaxHeight,
    Padding, Margin, BorderWidth, BorderRadius,
    FontSize, FontWeight, FontFamily, LineHeight,
    TextAlignment, TextWrap, LetterSpacing, WordSpacing,
    Opacity, Visibility, Display,
    Cursor, Shadow, Gradient,
    Custom
};

struct StyleProperty {
    StylePropertyType type;
    StyleValue value;
    bool important = false;
    std::optional<std::string> animation;
    
    template<typename T>
    std::optional<T> get_as() const {
        if (auto val = std::get_if<T>(&value)) {
            return *val;
        }
        return std::nullopt;
    }
};

// Color roles for theming
enum class ColorRole : uint8_t {
    Background, Foreground, Border,
    Highlight, Shadow, Text,
    Button, ButtonHover, ButtonPressed,
    Panel, Window, Dialog,
    Success, Warning, Error
};

// Theme class with comprehensive styling
class Theme {
private:
    std::string name_;
    std::unordered_map<std::string, StyleProperty> properties_;
    std::unordered_map<ColorRole, glm::vec4> colors_;
    
public:
    explicit Theme(std::string name = "default") : name_(std::move(name)) {
        // Initialize default colors
        colors_[ColorRole::Background] = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors_[ColorRole::Foreground] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        colors_[ColorRole::Border] = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        colors_[ColorRole::Text] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        colors_[ColorRole::Button] = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors_[ColorRole::ButtonHover] = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
        colors_[ColorRole::ButtonPressed] = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    }
    
    const std::string& name() const { return name_; }
    
    // Property management
    void set_property(const std::string& name, const StyleProperty& property) {
        properties_[name] = property;
    }
    
    std::optional<StyleProperty> get_property(const std::string& name) const {
        auto it = properties_.find(name);
        if (it != properties_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Color management
    glm::vec4 get_color(ColorRole role) const {
        auto it = colors_.find(role);
        if (it != colors_.end()) {
            return it->second;
        }
        return glm::vec4(1.0f); // Default white
    }
    
    void set_color(ColorRole role, const glm::vec4& color) {
        colors_[role] = color;
    }
    
    // Built-in themes
    void set_dark_theme() {
        colors_[ColorRole::Background] = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors_[ColorRole::Foreground] = glm::vec4(0.9f, 0.9f, 0.9f, 1.0f);
        colors_[ColorRole::Border] = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors_[ColorRole::Text] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        colors_[ColorRole::Button] = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        colors_[ColorRole::ButtonHover] = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors_[ColorRole::ButtonPressed] = glm::vec4(0.15f, 0.15f, 0.15f, 1.0f);
    }
    
    void set_light_theme() {
        colors_[ColorRole::Background] = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
        colors_[ColorRole::Foreground] = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        colors_[ColorRole::Border] = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors_[ColorRole::Text] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        colors_[ColorRole::Button] = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
        colors_[ColorRole::ButtonHover] = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
        colors_[ColorRole::ButtonPressed] = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
    }
    
    static std::unique_ptr<Theme> create_dark_theme();
    static std::unique_ptr<Theme> create_light_theme();
    static std::unique_ptr<Theme> create_professional_theme();
};

} // namespace Shotcut::UI
