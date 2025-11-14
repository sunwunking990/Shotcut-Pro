#pragma once

#include "layout_engine.h"
#include "theme_system.h"
#include <string>
#include <variant>
#include <coroutine>
#include <span>
#include <unordered_map>
#include <vector>
#include <optional>
#include <functional>
#include <memory>

// Forward declare Vulkan types to avoid including vulkan.h in headers
struct VkCommandBuffer_T;
typedef VkCommandBuffer_T* VkCommandBuffer;

namespace Shotcut::UI {

// Modern C++20 event system using variants and coroutines
enum class EventType : uint8_t {
    MouseMove, MouseDown, MouseUp, MouseWheel,
    KeyDown, KeyUp, TextInput,
    FocusGained, FocusLost,
    MouseEnter, MouseLeave,
    Resize, Move,
    TouchStart, TouchMove, TouchEnd,
    Custom
};

struct MouseEvent {
    glm::vec2 position;
    glm::vec2 delta;
    uint32_t button;
    uint32_t modifiers;
    float timestamp;
};

struct KeyEvent {
    uint32_t key_code;
    uint32_t scan_code;
    uint32_t modifiers;
    bool repeat;
    float timestamp;
};

struct TextInputEvent {
    std::string text;
    float timestamp;
};

using EventData = std::variant<
    MouseEvent, KeyEvent, TextInputEvent,
    glm::vec2, uint32_t, std::string
>;

struct Event {
    EventType type;
    EventData data;
    float timestamp;
    uint32_t source_id;
};

// Event handler using C++20 coroutines
template<typename T>
class EventHandler {
public:
    using handler_t = std::function<T(const Event&)>;
    
    void add_handler(handler_t handler) {
        handlers_.push_back(std::move(handler));
    }
    
    template<std::invocable<const Event&> F>
    requires std::same_as<std::invoke_result_t<F, const Event&>, T>
    void operator+=(F&& handler) {
        handlers_.emplace_back(std::forward<F>(handler));
    }
    
    std::vector<T> handle(const Event& event) const {
        std::vector<T> results;
        results.reserve(handlers_.size());
        
        for (const auto& handler : handlers_) {
            if (auto result = handler(event)) {
                results.push_back(*result);
            }
        }
        
        return results;
    }
    
private:
    std::vector<handler_t> handlers_;
};

// Widget base class with modern C++20 features
class Widget : public LayoutElement, public std::enable_shared_from_this<Widget> {
public:
    enum class State : uint8_t {
        Normal, Hovered, Pressed, Focused, Disabled
    };
    
    enum class Visibility : uint8_t {
        Visible, Hidden, Collapsed
    };
    
private:
    std::string id_;
    State state_ = State::Normal;
    Visibility visibility_ = Visibility::Visible;
    Rect bounds_;
    Rect clip_bounds_;
    bool needs_redraw_ = true;
    bool needs_layout_ = true;
    float opacity_ = 1.0f;
    
    // Event handling
    std::unordered_map<EventType, std::vector<std::function<void(const Event&)>>> event_handlers_;
    std::shared_ptr<Widget> parent_;
    std::vector<std::shared_ptr<Widget>> children_;
    
    // Styling
    std::shared_ptr<Theme> theme_;
    std::unordered_map<std::string, std::shared_ptr<StyleProperty>> custom_styles_;
    
public:
    explicit Widget(std::string id = "") : id_(std::move(id)) {}
    virtual ~Widget() = default;
    
    // Basic properties
    const std::string& id() const noexcept { return id_; }
    void set_id(std::string id) { id_ = std::move(id); }
    
    State state() const noexcept { return state_; }
    void set_state(State new_state) noexcept;
    
    Visibility visibility() const noexcept { return visibility_; }
    void set_visibility(Visibility vis) noexcept { 
        visibility_ = vis; 
        invalidate();
    }
    
    bool is_visible() const noexcept { return visibility_ == Visibility::Visible; }
    
    float opacity() const noexcept { return opacity_; }
    void set_opacity(float opacity) noexcept { 
        opacity_ = std::clamp(opacity, 0.0f, 1.0f);
        invalidate();
    }
    
    // LayoutElement interface implementation
    Rect get_bounds() const override { return bounds_; }
    void set_bounds(const Rect& bounds) override {
        if (bounds_ != bounds) {
            bounds_ = bounds;
            invalidate_layout();
        }
    }
    
    LayoutConstraints calculate_constraints() const override;
    void apply_constraints(const LayoutConstraints& constraints) override;
    
    void add_child(std::shared_ptr<LayoutElement> child) override;
    void remove_child(std::shared_ptr<LayoutElement> child) override;
    std::span<std::shared_ptr<LayoutElement>> get_children() override;
    
    void set_flex_properties(const FlexProperties& props) override;
    const FlexProperties& get_flex_properties() const override;
    
    void compute_layout(float available_width, float available_height) override;
    bool is_layout_valid() const override;
    
    // Event handling
    template<EventType Type, typename Handler>
    void add_event_handler(Handler&& handler) {
        event_handlers_[Type].emplace_back(std::forward<Handler>(handler));
    }
    
    void handle_event(const Event& event);
    bool handles_event(EventType type) const;
    
    // Rendering
    virtual void render(VkCommandBuffer command_buffer, const glm::mat4& transform);
    virtual void render_children(VkCommandBuffer command_buffer, const glm::mat4& transform);
    
    // Invalidations
    void invalidate() { needs_redraw_ = true; }
    void invalidate_layout();
    bool needs_redraw() const { return needs_redraw_; }
    bool needs_layout() const { return needs_layout_; }
    
    void mark_rendered() { needs_redraw_ = false; }
    void mark_layout_computed() { needs_layout_ = false; }
    
    // Hierarchy
    std::shared_ptr<Widget> parent() const { return parent_; }
    void set_parent(std::shared_ptr<Widget> parent) { parent_ = std::move(parent); }
    
    // Theming
    void set_theme(std::shared_ptr<Theme> theme) { theme_ = std::move(theme); }
    const Theme& theme() const { return *theme_; }
    
    void set_style(const std::string& property, const StyleProperty& value) {
        custom_styles_[property] = std::make_shared<StyleProperty>(value);
        invalidate();
    }
    
    std::optional<std::shared_ptr<StyleProperty>> get_style(const std::string& property) const;
    
    // Hit testing
    virtual std::shared_ptr<Widget> hit_test(const glm::vec2& point) const;
    virtual bool contains_point(const glm::vec2& point) const;
    
    // Focus management
    virtual bool can_accept_focus() const { return false; }
    virtual void on_focus_gained() {}
    virtual void on_focus_lost() {}
    
protected:
    // Virtual methods for customization
    virtual void on_state_changed(State old_state, State new_state) {}
    virtual void on_bounds_changed(const Rect& old_bounds, const Rect& new_bounds) {}
    virtual void on_event(const Event& event) {}
    virtual void on_theme_changed() {}
    
    // Rendering helpers
    virtual void render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) = 0;
    glm::mat4 get_transform() const;
    
private:
    void propagate_event_to_children(const Event& event);
    std::shared_ptr<Widget> find_child_at_point(const glm::vec2& point) const;
};

// Concrete widget implementations
class Button : public Widget {
private:
    std::string text_;
    std::function<void()> on_click_;
    glm::vec4 text_color_ = glm::vec4(1.0f);
    glm::vec4 background_color_ = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    glm::vec4 hover_color_ = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);
    glm::vec4 pressed_color_ = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    float border_radius_ = 4.0f;
    float border_width_ = 1.0f;
    
public:
    explicit Button(std::string text = "", std::function<void()> on_click = nullptr)
        : Widget("button"), text_(std::move(text)), on_click_(std::move(on_click)) {
        
        add_event_handler<EventType::MouseDown>([this](const Event& event) {
            if (const auto* mouse_event = std::get_if<MouseEvent>(&event.data)) {
                if (mouse_event->button == 1) { // Left click
                    set_state(State::Pressed);
                    return true;
                }
            }
            return false;
        });
        
        add_event_handler<EventType::MouseUp>([this](const Event& event) {
            if (const auto* mouse_event = std::get_if<MouseEvent>(&event.data)) {
                if (mouse_event->button == 1 && state() == State::Pressed) {
                    if (on_click_) on_click_();
                    set_state(State::Hovered);
                    return true;
                }
            }
            return false;
        });
        
        add_event_handler<EventType::MouseEnter>([this](const Event& event) {
            if (state() == State::Normal) {
                set_state(State::Hovered);
            }
            return true;
        });
        
        add_event_handler<EventType::MouseLeave>([this](const Event& event) {
            set_state(State::Normal);
            return true;
        });
    }
    
    const std::string& text() const { return text_; }
    void set_text(std::string text) {
        text_ = std::move(text);
        invalidate();
    }
    
    glm::vec4 text_color() const { return text_color_; }
    void set_text_color(const glm::vec4& color) {
        text_color_ = color;
        invalidate();
    }
    
    glm::vec4 background_color() const { return background_color_; }
    void set_background_color(const glm::vec4& color) {
        background_color_ = color;
        invalidate();
    }
    
    float border_radius() const { return border_radius_; }
    void set_border_radius(float radius) {
        border_radius_ = radius;
        invalidate();
    }
    
    bool can_accept_focus() const override { return true; }
    
protected:
    void on_state_changed(State old_state, State new_state) override;
    void render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) override;
};

class Panel : public Widget {
private:
    glm::vec4 background_color_ = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    float border_radius_ = 0.0f;
    float border_width_ = 0.0f;
    glm::vec4 border_color_ = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    bool scrollable_ = false;
    glm::vec2 scroll_offset_ = glm::vec2(0.0f);
    
public:
    explicit Panel(std::string id = "panel") : Widget(std::move(id)) {}
    
    glm::vec4 background_color() const { return background_color_; }
    void set_background_color(const glm::vec4& color) {
        background_color_ = color;
        invalidate();
    }
    
    float border_radius() const { return border_radius_; }
    void set_border_radius(float radius) {
        border_radius_ = radius;
        invalidate();
    }
    
    bool scrollable() const { return scrollable_; }
    void set_scrollable(bool scrollable) { scrollable_ = scrollable; }
    
    glm::vec2 scroll_offset() const { return scroll_offset_; }
    void set_scroll_offset(const glm::vec2& offset) {
        scroll_offset_ = offset;
        invalidate();
    }
    
protected:
    void render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) override;
    void handle_scroll_event(const MouseEvent& event);
};

} // namespace Shotcut::UI
