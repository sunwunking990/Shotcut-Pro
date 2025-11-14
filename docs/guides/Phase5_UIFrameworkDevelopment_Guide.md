# Phase 5: UI Framework Development Guide - Shotcut C++ Transformation

**Document Created:** November 13, 2025  
**Implementation Focus:** Building Professional-Grade Custom UI Framework  
**Duration:** Months 4-6 (UI Development Phase)  
**Prerequisites:** Phase 4 completed (Foundation Implementation)

---

## PHASE 5 OVERVIEW

Phase 5 focuses on creating a professional-grade custom UI framework that rivals industry standards like Adobe Premiere Pro and DaVinci Resolve. This framework will be built entirely in modern C++20/23 with Vulkan rendering, providing pixel-perfect control, GPU acceleration, and the professional "feel" that commercial editors achieve through proprietary interfaces.

**Critical Success Factors:**
- **Professional UX Design:** Pixel-perfect rendering with immediate mode GUI concepts
- **GPU-Accelerated Everything:** All UI elements rendered through Vulkan pipeline
- **Modern C++20/23 Architecture:** Modules, concepts, ranges, and coroutines throughout
- **Performance-First:** 60fps UI responsiveness even with complex timelines
- **Extensible Framework:** Plugin-ready architecture for third-party extensions
- **Professional Look:** Custom styling system matching commercial editors

---

## 5.1 CUSTOM LAYOUT ENGINE IMPLEMENTATION

### 5.1.1 Modern Layout System Architecture

**Layout Engine Design Philosophy:**
- **Immediate Mode with Retained Elements:** Best of both worlds for performance and state management
- **Flexbox-Inspired Layout:** Familiar CSS-like layout rules for professional designers
- **GPU-Accelerated Layout:** Layout calculations performed on GPU when possible
- **Constraint-Based System:** Declarative layout with automatic constraint resolution

**src/ui/layout_engine.h:**
```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <optional>
#include <concepts>
#include <ranges>
#include <functional>

namespace Shotcut::UI {

// Modern C++20 concepts for layout elements
template<typename T>
concept LayoutElement = requires(T t) {
    { t.get_bounds() } -> std::same_as<Rect>;
    { t.set_bounds(Rect{}) } -> std::same_as<void>;
    { t.calculate_constraints() } -> std::same_as<LayoutConstraints>;
    { t.apply_constraints(LayoutConstraints{}) } -> std::same_as<void>;
};

// Layout constraints system
struct LayoutConstraints {
    std::optional<float> min_width, max_width;
    std::optional<float> min_height, max_height;
    std::optional<float> preferred_width, preferred_height;
    
    bool has_flexible_width() const noexcept {
        return !min_width.has_value() || !max_width.has_value() || 
               (*max_width - *min_width) > 0.1f;
    }
    
    bool has_flexible_height() const noexcept {
        return !min_height.has_value() || !max_height.has_value() || 
               (*max_height - *min_height) > 0.1f;
    }
};

// Flex-like layout properties
struct FlexProperties {
    enum class Direction : uint8_t {
        Row, RowReverse, Column, ColumnReverse
    } direction = Direction::Row;
    
    enum class Wrap : uint8_t {
        NoWrap, Wrap, WrapReverse
    } wrap = Wrap::NoWrap;
    
    enum class JustifyContent : uint8_t {
        FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly
    } justify_content = JustifyContent::FlexStart;
    
    enum class AlignItems : uint8_t {
        FlexStart, FlexEnd, Center, Stretch, Baseline
    } align_items = AlignItems::Stretch;
    
    enum class AlignContent : uint8_t {
        FlexStart, FlexEnd, Center, Stretch, SpaceBetween, SpaceAround
    } align_content = AlignContent::Stretch;
    
    float gap = 0.0f;
};

// Layout element base class
class LayoutElement {
public:
    virtual ~LayoutElement() = default;
    
    // Core layout interface
    virtual Rect get_bounds() const = 0;
    virtual void set_bounds(const Rect& bounds) = 0;
    virtual LayoutConstraints calculate_constraints() const = 0;
    virtual void apply_constraints(const LayoutConstraints& constraints) = 0;
    
    // Hierarchy management
    virtual void add_child(std::shared_ptr<LayoutElement> child) = 0;
    virtual void remove_child(std::shared_ptr<LayoutElement> child) = 0;
    virtual std::span<std::shared_ptr<LayoutElement>> get_children() = 0;
    
    // Layout properties
    virtual void set_flex_properties(const FlexProperties& props) = 0;
    virtual const FlexProperties& get_flex_properties() const = 0;
    
    // Layout computation
    virtual void compute_layout(float available_width, float available_height) = 0;
    virtual void invalidate_layout() = 0;
    virtual bool is_layout_valid() const = 0;
};

// Container element for children
class Container : public LayoutElement {
private:
    std::vector<std::shared_ptr<LayoutElement>> children_;
    FlexProperties flex_props_;
    Rect bounds_;
    bool layout_valid_ = false;
    
public:
    Container() = default;
    
    // LayoutElement interface
    Rect get_bounds() const override { return bounds_; }
    void set_bounds(const Rect& bounds) override { 
        bounds_ = bounds; 
        invalidate_layout();
    }
    
    LayoutConstraints calculate_constraints() const override;
    void apply_constraints(const LayoutConstraints& constraints) override;
    
    void add_child(std::shared_ptr<LayoutElement> child) override;
    void remove_child(std::shared_ptr<LayoutElement> child) override;
    std::span<std::shared_ptr<LayoutElement>> get_children() override { 
        return children_; 
    }
    
    void set_flex_properties(const FlexProperties& props) override {
        flex_props_ = props;
        invalidate_layout();
    }
    
    const FlexProperties& get_flex_properties() const override { 
        return flex_props_; 
    }
    
    void compute_layout(float available_width, float available_height) override;
    void invalidate_layout() override { layout_valid_ = false; }
    bool is_layout_valid() const override { return layout_valid_; }
    
private:
    struct FlexItem {
        std::shared_ptr<LayoutElement> element;
        float flex_grow = 0.0f;
        float flex_shrink = 1.0f;
        float flex_basis = 0.0f;
        LayoutConstraints constraints;
    };
    
    std::vector<FlexItem> prepare_flex_items() const;
    void compute_flex_layout(const std::vector<FlexItem>& items, 
                           float available_width, float available_height);
    float distribute_flexible_space(const std::vector<FlexItem>& items, 
                               float available_space, bool is_width) const;
};

// GPU-accelerated layout computation
class GPULayoutEngine {
private:
    VulkanContext* vulkan_context_;
    VkDescriptorSetLayout descriptor_set_layout_;
    VkPipeline compute_pipeline_;
    VkBuffer layout_buffer_;
    VkDeviceMemory layout_buffer_memory_;
    
    struct LayoutComputeData {
        glm::vec4 bounds;           // x, y, width, height
        glm::vec4 constraints;      // min_w, max_w, min_h, max_h
        glm::vec4 flex_props;       // direction, wrap, justify, align
        uint32_t child_count;
        uint32_t padding[3];
    };
    
public:
    GPULayoutEngine(VulkanContext* context);
    ~GPULayoutEngine();
    
    bool initialize();
    void cleanup();
    
    // GPU-accelerated layout computation
    void compute_layout_async(std::span<LayoutElement*> elements, 
                          std::function<void()> completion_callback);
    
    // CPU fallback
    void compute_layout_cpu(std::span<LayoutElement*> elements);
    
private:
    bool create_compute_pipeline();
    bool create_layout_buffers();
    void update_layout_buffer(const LayoutComputeData& data);
};

} // namespace Shotcut::UI
```

### 5.1.2 Layout Computation Algorithm

**src/ui/layout_engine.cpp:**
```cpp
#include "layout_engine.h"
#include <algorithm>
#include <numeric>

namespace Shotcut::UI {

LayoutConstraints Container::calculate_constraints() const {
    LayoutConstraints result;
    
    if (children_.empty()) {
        result.min_width = result.max_width = 0.0f;
        result.min_height = result.max_height = 0.0f;
        return result;
    }
    
    // Calculate combined constraints based on flex direction
    float total_min_width = 0.0f, total_max_width = 0.0f;
    float total_min_height = 0.0f, total_max_height = 0.0f;
    float max_child_min_width = 0.0f, max_child_min_height = 0.0f;
    
    for (const auto& child : children_) {
        auto child_constraints = child->calculate_constraints();
        
        // For row layout
        if (flex_props_.direction == FlexProperties::Direction::Row ||
            flex_props_.direction == FlexProperties::Direction::RowReverse) {
            
            total_min_width += child_constraints.min_width.value_or(0.0f);
            total_max_width += child_constraints.max_width.value_or(std::numeric_limits<float>::max());
            max_child_min_height = std::max(max_child_min_height, 
                                        child_constraints.min_height.value_or(0.0f));
        } 
        // For column layout
        else {
            total_min_height += child_constraints.min_height.value_or(0.0f);
            total_max_height += child_constraints.max_height.value_or(std::numeric_limits<float>::max());
            max_child_min_width = std::max(max_child_min_width, 
                                       child_constraints.min_width.value_or(0.0f));
        }
    }
    
    // Apply flex properties
    if (flex_props_.direction == FlexProperties::Direction::Row ||
        flex_props_.direction == FlexProperties::Direction::RowReverse) {
        
        result.min_width = total_min_width + (children_.size() - 1) * flex_props_.gap;
        result.max_width = total_max_width + (children_.size() - 1) * flex_props_.gap;
        result.min_height = max_child_min_height;
        result.max_height = std::nullopt; // Flexible height by default
    } else {
        result.min_height = total_min_height + (children_.size() - 1) * flex_props_.gap;
        result.max_height = total_max_height + (children_.size() - 1) * flex_props_.gap;
        result.min_width = max_child_min_width;
        result.max_width = std::nullopt; // Flexible width by default
    }
    
    return result;
}

void Container::compute_layout(float available_width, float available_height) {
    if (layout_valid_ && children_.empty()) return;
    
    // Prepare flex items
    auto flex_items = prepare_flex_items();
    if (flex_items.empty()) return;
    
    // Compute flex layout
    compute_flex_layout(flex_items, available_width, available_height);
    
    layout_valid_ = true;
}

std::vector<Container::FlexItem> Container::prepare_flex_items() const {
    std::vector<FlexItem> items;
    items.reserve(children_.size());
    
    for (const auto& child : children_) {
        FlexItem item;
        item.element = child;
        item.constraints = child->calculate_constraints();
        item.flex_basis = item.constraints.preferred_width.value_or(0.0f);
        
        items.push_back(std::move(item));
    }
    
    return items;
}

void Container::compute_flex_layout(const std::vector<FlexItem>& items, 
                                float available_width, float available_height) {
    
    const bool is_row = (flex_props_.direction == FlexProperties::Direction::Row ||
                        flex_props_.direction == FlexProperties::Direction::RowReverse);
    
    const float main_axis_size = is_row ? available_width : available_height;
    const float cross_axis_size = is_row ? available_height : available_width;
    
    // Calculate total flexible space
    float total_flexible_space = 0.0f;
    for (const auto& item : items) {
        if (is_row && item.constraints.has_flexible_width()) {
            total_flexible_space += item.flex_grow;
        } else if (!is_row && item.constraints.has_flexible_height()) {
            total_flexible_space += item.flex_grow;
        }
    }
    
    // Distribute space and position items
    float current_pos = 0.0f;
    const float gap = flex_props_.gap;
    
    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        Rect child_bounds;
        
        if (is_row) {
            // Calculate width
            float child_width = item.flex_basis;
            if (item.constraints.has_flexible_width() && total_flexible_space > 0.0f) {
                float extra_space = main_axis_size - child_width;
                child_width += extra_space * (item.flex_grow / total_flexible_space);
            }
            
            // Calculate height
            float child_height = cross_axis_size;
            if (item.constraints.min_height) {
                child_height = std::max(child_height, *item.constraints.min_height);
            }
            if (item.constraints.max_height) {
                child_height = std::min(child_height, *item.constraints.max_height);
            }
            
            child_bounds = {current_pos, 0.0f, child_width, child_height};
            current_pos += child_width + gap;
        } else {
            // Calculate height
            float child_height = item.flex_basis;
            if (item.constraints.has_flexible_height() && total_flexible_space > 0.0f) {
                float extra_space = main_axis_size - child_height;
                child_height += extra_space * (item.flex_grow / total_flexible_space);
            }
            
            // Calculate width
            float child_width = cross_axis_size;
            if (item.constraints.min_width) {
                child_width = std::max(child_width, *item.constraints.min_width);
            }
            if (item.constraints.max_width) {
                child_width = std::min(child_width, *item.constraints.max_width);
            }
            
            child_bounds = {0.0f, current_pos, child_width, child_height};
            current_pos += child_height + gap;
        }
        
        item.element->set_bounds(child_bounds);
        item.element->apply_constraints(item.constraints);
    }
}

} // namespace Shotcut::UI
```

---

## 5.2 BASIC WIDGET SYSTEM IMPLEMENTATION

### 5.2.1 Widget Architecture with Modern C++20

**Widget Design Philosophy:**
- **Component-Based Architecture:** Each widget is a self-contained component
- **Immediate Mode Rendering:** Fast, GPU-accelerated drawing
- **Event-Driven Architecture:** Modern event handling with coroutines
- **Theme System:** Comprehensive theming and styling
- **Accessibility Support:** Built-in accessibility from day one

**src/ui/widget_system.h:**
```cpp
#pragma once

#include "layout_engine.h"
#include <vulkan/vulkan.h>
#include <string>
#include <variant>
#include <coroutine>
#include <span>

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
    std::unordered_map<std::string, StyleProperty> custom_styles_;
    
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
    void invalidate_layout() override;
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
    void invalidate_layout() { needs_layout_ = true; }
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
        custom_styles_[property] = value;
        invalidate();
    }
    
    std::optional<StyleProperty> get_style(const std::string& property) const;
    
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

// Button widget with modern styling
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
        
        add_event_handler<EventType::MouseEnter>([this](const Event&) {
            if (state() == State::Normal) {
                set_state(State::Hovered);
            }
            return true;
        });
        
        add_event_handler<EventType::MouseLeave>([this](const Event&) {
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

// Panel widget for grouping
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
```

### 5.2.2 Advanced Text Widget with FreeType Integration

**src/ui/text_widget.h:**
```cpp
#pragma once

#include "widget_system.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>
#include <unicode/unistr.h>
#include <unicode/ubidi.h>
#include <unordered_map>

namespace Shotcut::UI {

// Modern text rendering with Unicode and complex script support
class TextWidget : public Widget {
private:
    std::string text_;
    std::u32string unicode_text_;
    std::vector<glm::vec2> glyph_positions_;
    std::vector<uint32_t> glyph_indices_;
    
    // Font handling
    struct FontData {
        FT_Face face;
        hb_font_t* hb_font;
        uint32_t size;
        std::unordered_map<uint32_t, GlyphMetrics> glyphs;
    };
    
    std::shared_ptr<FontData> font_;
    float font_size_ = 12.0f;
    glm::vec4 color_ = glm::vec4(1.0f);
    
    // Text layout
    enum class TextAlignment : uint8_t {
        Left, Center, Right, Justify
    } alignment_ = TextAlignment::Left;
    
    enum class TextWrap : uint8_t {
        None, Word, Character
    } wrap_ = TextWrap::None;
    
    float line_height_ = 1.2f;
    float letter_spacing_ = 0.0f;
    float word_spacing_ = 0.0f;
    
    // Bidirectional text support
    std::unique_ptr<icu::UnicodeString> icu_text_;
    std::unique_ptr<icu::Bidi> bidi_processor_;
    
    // GPU resources
    VkBuffer vertex_buffer_;
    VkDeviceMemory vertex_buffer_memory_;
    VkBuffer index_buffer_;
    VkDeviceMemory index_buffer_memory_;
    VkImage glyph_atlas_;
    VkDeviceMemory glyph_atlas_memory_;
    VkImageView glyph_atlas_view_;
    VkSampler glyph_sampler_;
    
    bool text_dirty_ = true;
    bool gpu_resources_dirty_ = true;
    
    struct GlyphMetrics {
        glm::vec2 bearing;
        glm::vec2 size;
        glm::vec2 advance;
        glm::vec2 uv_offset;
        glm::vec2 uv_size;
        uint32_t atlas_x, atlas_y;
    };
    
public:
    explicit TextWidget(std::string text = "", std::string id = "text");
    ~TextWidget() override;
    
    const std::string& text() const { return text_; }
    void set_text(std::string text);
    
    float font_size() const { return font_size_; }
    void set_font_size(float size);
    
    glm::vec4 color() const { return color_; }
    void set_color(const glm::vec4& color) {
        color_ = color;
        invalidate();
    }
    
    TextAlignment alignment() const { return alignment_; }
    void set_alignment(TextAlignment align) {
        alignment_ = align;
        invalidate_layout();
    }
    
    TextWrap wrap() const { return wrap_; }
    void set_wrap(TextWrap wrap) {
        wrap_ = wrap;
        invalidate_layout();
    }
    
    float line_height() const { return line_height_; }
    void set_line_height(float height) {
        line_height_ = height;
        invalidate_layout();
    }
    
    LayoutConstraints calculate_constraints() const override;
    
protected:
    void render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) override;
    void on_bounds_changed(const Rect& old_bounds, const Rect& new_bounds) override;
    
private:
    void update_unicode_text();
    void process_bidirectional_text();
    void shape_text_with_harfbuzz();
    void layout_glyphs();
    void update_gpu_resources();
    void create_glyph_atlas();
    void upload_vertex_data();
    
    GlyphMetrics get_or_create_glyph(uint32_t codepoint);
    glm::vec2 measure_text() const;
    void calculate_line_breaks(std::vector<size_t>& line_breaks) const;
};

// Font manager for efficient font loading and caching
class FontManager {
private:
    static std::unique_ptr<FontManager> instance_;
    FT_Library ft_library_;
    std::unordered_map<std::string, std::shared_ptr<TextWidget::FontData>> font_cache_;
    
public:
    static FontManager& get() {
        if (!instance_) {
            instance_ = std::make_unique<FontManager>();
        }
        return *instance_;
    }
    
    bool initialize();
    void cleanup();
    
    std::shared_ptr<TextWidget::FontData> load_font(const std::string& font_path, uint32_t size);
    void unload_font(const std::string& font_path, uint32_t size);
    
private:
    FontManager() = default;
    ~FontManager() = default;
    
    std::string get_font_key(const std::string& font_path, uint32_t size) const;
};

} // namespace Shotcut::UI
```

---

## 5.3 TEXT RENDERING WITH FREETYPE/HARFBUZZ

### 5.3.1 Advanced Text Rendering Pipeline

**Text Rendering Architecture:**
- **Unicode Support:** Full Unicode with complex scripts (Arabic, Hebrew, Devanagari)
- **Bidirectional Text:** ICU-based bidirectional algorithm
- **Font Shaping:** HarfBuzz for proper glyph positioning
- **GPU Acceleration:** Glyph atlas with distance field rendering
- **Font Fallback:** Automatic font fallback for missing glyphs

**src/ui/text_renderer.h:**
```cpp
#pragma once

#include "text_widget.h"
#include <vulkan/vulkan.h>
#include <array>
#include <memory>
#include <mutex>

namespace Shotcut::UI {

class TextRenderer {
private:
    VulkanContext* vulkan_context_;
    
    // Rendering pipeline
    VkPipeline text_pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    
    // Uniform buffers
    struct UniformBufferObject {
        glm::mat4 projection;
        glm::mat4 model;
        glm::vec4 color;
        glm::vec4 text_params; // x: font size, y: pixel range, z: edge softness, w: reserved
    };
    
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers_;
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> uniform_buffers_memory_;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> uniform_buffers_mapped_;
    
    // Glyph atlas management
    struct GlyphAtlas {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        VkSampler sampler;
        uint32_t width, height;
        uint32_t cursor_x, cursor_y;
        uint32_t row_height;
        std::mutex mutex;
        bool needs_rebuild;
    };
    
    std::unique_ptr<GlyphAtlas> glyph_atlas_;
    
    // Distance field generation
    VkComputePipeline distance_field_pipeline_;
    VkDescriptorSetLayout df_descriptor_layout_;
    
public:
    TextRenderer(VulkanContext* context);
    ~TextRenderer();
    
    bool initialize();
    void cleanup();
    
    // Text rendering
    void render_text(const std::string& text, const glm::vec2& position, 
                   float font_size, const glm::vec4& color,
                   VkCommandBuffer command_buffer, uint32_t current_frame);
    
    // Glyph management
    bool add_glyph_to_atlas(uint32_t codepoint, const FT_Face& face, 
                          TextWidget::GlyphMetrics& metrics);
    
    // Distance field generation
    void generate_distance_field(const uint8_t* bitmap, uint32_t width, uint32_t height,
                             uint8_t* output, uint32_t output_width, uint32_t output_height);
    
private:
    bool create_text_pipeline();
    bool create_descriptor_sets();
    bool create_uniform_buffers();
    bool create_glyph_atlas();
    bool create_distance_field_pipeline();
    
    void update_uniform_buffer(uint32_t current_image, const glm::mat4& projection,
                           const glm::mat4& model, const glm::vec4& color,
                           float font_size);
    
    VkDescriptorSet get_descriptor_set(uint32_t current_frame);
};

// Text layout engine for complex scripts
class TextLayoutEngine {
private:
    hb_buffer_t* hb_buffer_;
    hb_font_t* hb_font_;
    hb_feature_t* hb_features_;
    size_t hb_features_count_;
    
    // Bidi processing
    std::unique_ptr<icu::UnicodeString> unicode_text_;
    std::unique_ptr<icu::Bidi> bidi_processor_;
    std::unique_ptr<icu::BidiParagraph> bidi_paragraph_;
    
public:
    TextLayoutEngine();
    ~TextLayoutEngine();
    
    bool initialize(hb_font_t* font);
    
    // Text shaping and layout
    std::vector<TextWidget::GlyphMetrics> shape_text(
        const std::u32string& text, const std::string& language, 
        const std::string& script, hb_direction_t direction
    );
    
    // Bidi processing
    std::vector<hb_script_t> detect_scripts(const std::u32string& text);
    hb_direction_t resolve_base_direction(const std::u32string& text);
    
    // Line breaking
    std::vector<size_t> find_line_breaks(const std::u32string& text, 
                                       float max_width, float font_size);
    
private:
    void setup_harfbuzz_features();
    hb_script_t icu_script_to_hb(icu::UScriptCode script);
};

} // namespace Shotcut::UI
```

### 5.3.2 Distance Field Text Rendering

**Distance Field Rendering for High-Quality Text:**
- **Signed Distance Fields:** GPU-accelerated distance field generation
- **Multi-Resolution Scaling:** Sharp text at any scale
- **Anti-Aliasing:** Smooth edges at all zoom levels
- **Performance Benefits:** Single atlas for all text sizes

---

## 5.4 STYLING AND THEMING SYSTEM

### 5.4.1 Professional Theming Architecture

**Theme System Design:**
- **CSS-Inspired Syntax:** Familiar styling for designers
- **Component-Based Theming:** Widget-specific styling
- **Dynamic Theme Switching:** Runtime theme changes
- **Dark/Light Mode Support:** Professional appearance
- **Custom Property Support:** Extensible styling system

**src/ui/theme_system.h:**
```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <memory>

namespace Shotcut::UI {

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

// CSS-like selector system
enum class SelectorType : uint8_t {
    Universal,     // *
    Type,          // button, panel, text
    Class,         // .primary, .secondary
    ID,            // #header, #content
    Attribute,     // [disabled], [state="hover"]
    PseudoClass,   // :hover, :active, :focus
    PseudoElement, // ::before, ::after
    Descendant,    // panel button
    Child,         // panel > button
    Adjacent,      // button + button
    Sibling        // button ~ button
};

struct Selector {
    SelectorType type;
    std::string value;
    uint32_t specificity;
    
    // Combinator support
    enum class Combinator : uint8_t {
        None, Descendant, Child, Adjacent, Sibling
    } combinator = Combinator::None;
    
    std::unique_ptr<Selector> next;
    
    uint32_t calculate_specificity() const;
    bool matches(const Widget& widget) const;
};

// Style rule with selectors and properties
struct StyleRule {
    std::vector<std::unique_ptr<Selector>> selectors;
    std::unordered_map<std::string, StyleProperty> properties;
    uint32_t order; // For cascade order
};

// Animation system
struct Animation {
    std::string name;
    std::unordered_map<std::string, std::vector<StyleProperty>> keyframes;
    float duration = 0.0f;
    float delay = 0.0f;
    enum class Easing { Linear, Ease, EaseIn, EaseOut, EaseInOut } easing = Easing::Linear;
    int iteration_count = 1;
    enum class Direction { Normal, Alternate, Reverse, AlternateReverse } direction = Direction::Normal;
    enum class FillMode { None, Forwards, Backwards, Both } fill_mode = FillMode::None;
};

// Theme class with comprehensive styling
class Theme {
private:
    std::string name_;
    std::vector<StyleRule> rules_;
    std::unordered_map<std::string, Animation> animations_;
    std::unordered_map<std::string, StyleValue> variables_;
    
    // Compiled theme for faster lookups
    std::unordered_map<std::string, std::vector<const StyleRule*>> compiled_rules_;
    
public:
    explicit Theme(std::string name = "default") : name_(std::move(name)) {}
    
    const std::string& name() const { return name_; }
    
    // Rule management
    void add_rule(StyleRule rule);
    void remove_rule(const std::string& selector);
    const std::vector<StyleRule>& rules() const { return rules_; }
    
    // Variable system
    void set_variable(const std::string& name, const StyleValue& value);
    std::optional<StyleValue> get_variable(const std::string& name) const;
    
    // Animation management
    void add_animation(const Animation& animation);
    std::optional<const Animation*> get_animation(const std::string& name) const;
    
    // Style resolution
    std::unordered_map<std::string, StyleProperty> resolve_styles(
        const Widget& widget, const std::unordered_map<std::string, StyleProperty>& inline_styles = {}
    ) const;
    
    // Theme compilation
    void compile();
    bool is_compiled() const { return !compiled_rules_.empty(); }
    
    // Loading and saving
    bool load_from_file(const std::string& filename);
    bool save_to_file(const std::string& filename) const;
    
    // Built-in themes
    static std::unique_ptr<Theme> create_dark_theme();
    static std::unique_ptr<Theme> create_light_theme();
    static std::unique_ptr<Theme> create_professional_theme();
    
private:
    std::vector<const StyleRule*> find_matching_rules(const Widget& widget) const;
    void resolve_variables(StyleProperty& property) const;
    uint32_t calculate_rule_specificity(const Selector& selector) const;
};

// Theme manager for runtime theme switching
class ThemeManager {
private:
    static std::unique_ptr<ThemeManager> instance_;
    std::unordered_map<std::string, std::unique_ptr<Theme>> themes_;
    std::shared_ptr<Theme> active_theme_;
    std::shared_ptr<Theme> fallback_theme_;
    
public:
    static ThemeManager& get() {
        if (!instance_) {
            instance_ = std::make_unique<ThemeManager>();
        }
        return *instance_;
    }
    
    // Theme management
    void register_theme(std::unique_ptr<Theme> theme);
    void unregister_theme(const std::string& name);
    
    std::shared_ptr<Theme> get_theme(const std::string& name);
    std::shared_ptr<Theme> active_theme() const { return active_theme_; }
    
    void set_active_theme(const std::string& name);
    void set_active_theme(std::shared_ptr<Theme> theme);
    
    // Built-in theme registration
    void register_builtin_themes();
    
private:
    ThemeManager() = default;
    ~ThemeManager() = default;
};

} // namespace Shotcut::UI
```

### 5.4.2 Professional Theme Definitions

**Built-in Professional Themes:**
- **Dark Professional:** Modern dark theme matching industry standards
- **Light Professional:** Clean light theme for well-lit environments
- **High Contrast:** Accessibility-focused theme
- **Custom Themes:** User-defined themes with CSS-like syntax

---

## 5.5 FOCUS AND INPUT MANAGEMENT

### 5.5.1 Advanced Input System

**Input Management Architecture:**
- **Unified Input Pipeline:** Single source for all input events
- **Event Filtering and Routing:** Flexible event handling
- **Input Method Support:** International text input
- **Multi-Touch Support:** Modern touch devices
- **Gesture Recognition:** Pinch, swipe, rotate gestures

**src/ui/input_manager.h:**
```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <deque>
#include <unordered_map>
#include <memory>
#include <functional>
#include <coroutine>

namespace Shotcut::UI {

enum class InputDevice : uint8_t {
    Mouse, Keyboard, Touch, Gamepad, Pen
};

enum class InputState : uint8_t {
    Pressed, Released, Repeated, Moved, Scrolled
};

struct InputEvent {
    InputDevice device;
    InputState state;
    float timestamp;
    uint32_t modifiers;
    
    union {
        struct {
            glm::vec2 position;
            glm::vec2 delta;
            uint32_t button;
            float scroll_delta;
        } mouse;
        
        struct {
            uint32_t key_code;
            uint32_t scan_code;
            std::string text;
        } keyboard;
        
        struct {
            glm::vec2 position;
            float pressure;
            float radius;
            uint32_t id;
        } touch;
        
        struct {
            uint32_t gamepad_id;
            uint32_t button;
            glm::vec2 axis;
        } gamepad;
        
        struct {
            glm::vec2 position;
            float pressure;
            float tilt_x, tilt_y;
            uint32_t buttons;
        } pen;
    };
};

// Modern C++20 generator for input events
class InputEventGenerator {
public:
    struct promise_type {
        InputEvent current_value_;
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() { return {}; }
        
        InputEventGenerator get_return_object() {
            return InputEventGenerator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        
        void unhandled_exception() {}
        void return_void() {}
        
        std::suspend_always yield_value(InputEvent value) {
            current_value_ = std::move(value);
            return {};
        }
    };
    
private:
    std::coroutine_handle<promise_type> coro_;
    
public:
    InputEventGenerator(std::coroutine_handle<promise_type> coro) : coro_(coro) {}
    
    InputEvent operator()() {
        coro_.resume();
        return std::move(coro_.promise().current_value_);
    }
    
    bool done() const {
        return coro_.done();
    }
};

// Input manager with comprehensive device support
class InputManager {
private:
    std::deque<InputEvent> event_queue_;
    std::unordered_map<uint32_t, std::function<bool(const InputEvent&)>> event_filters_;
    std::unordered_map<uint32_t, std::function<void(const InputEvent&)>> event_handlers_;
    
    // Device state tracking
    struct MouseState {
        glm::vec2 position;
        glm::vec2 last_position;
        uint32_t buttons = 0;
        uint32_t last_buttons = 0;
        float scroll_accumulator = 0.0f;
    } mouse_state_;
    
    struct KeyboardState {
        std::array<bool, 256> keys_pressed{};
        std::array<bool, 256> keys_repeated{};
        uint32_t modifiers = 0;
        std::string compose_text;
    } keyboard_state_;
    
    struct TouchState {
        std::array<bool, 10> touches_active{};
        std::array<glm::vec2, 10> touch_positions;
        std::array<float, 10> touch_pressures;
    } touch_state_;
    
    // Gesture recognition
    struct GestureState {
        bool pinch_active = false;
        float initial_distance = 0.0f;
        float current_distance = 0.0f;
        glm::vec2 center_point;
        
        bool rotate_active = false;
        float initial_angle = 0.0f;
        float current_angle = 0.0f;
    } gesture_state_;
    
    // Input mapping and configuration
    std::unordered_map<std::string, uint32_t> key_bindings_;
    std::unordered_map<std::string, std::function<void()>> action_bindings_;
    
public:
    InputManager() = default;
    ~InputManager() = default;
    
    // Event processing
    void update(float delta_time);
    void queue_event(InputEvent event);
    
    // Event handling
    uint32_t add_event_filter(std::function<bool(const InputEvent&)> filter);
    void remove_event_filter(uint32_t filter_id);
    
    uint32_t add_event_handler(std::function<void(const InputEvent&)> handler);
    void remove_event_handler(uint32_t handler_id);
    
    // Direct input queries
    glm::vec2 get_mouse_position() const { return mouse_state_.position; }
    glm::vec2 get_mouse_delta() const { 
        return mouse_state_.position - mouse_state_.last_position; 
    }
    bool is_mouse_button_pressed(uint32_t button) const {
        return (mouse_state_.buttons & (1 << button)) != 0;
    }
    
    bool is_key_pressed(uint32_t key_code) const {
        return key_code < 256 && keyboard_state_.keys_pressed[key_code];
    }
    
    bool are_modifiers_pressed(uint32_t modifiers) const {
        return (keyboard_state_.modifiers & modifiers) == modifiers;
    }
    
    // Input method support
    void start_composition();
    void set_composition_text(const std::string& text);
    void commit_composition();
    void cancel_composition();
    
    // Key binding system
    void set_key_binding(const std::string& action, uint32_t key_code, uint32_t modifiers = 0);
    void set_action_binding(const std::string& action, std::function<void()> callback);
    bool trigger_action(const std::string& action);
    
    // Generator for coroutines
    InputEventGenerator get_event_generator();
    
private:
    void process_mouse_events();
    void process_keyboard_events();
    void process_touch_events();
    void process_gestures();
    
    void update_mouse_state(const InputEvent& event);
    void update_keyboard_state(const InputEvent& event);
    void update_touch_state(const InputEvent& event);
    
    bool detect_pinch_gesture();
    bool detect_rotate_gesture();
    void dispatch_gesture_event(const std::string& gesture, const glm::vec2& center, float value);
};

// Focus management system
class FocusManager {
private:
    std::shared_ptr<Widget> focused_widget_;
    std::vector<std::shared_ptr<Widget>> focus_chain_;
    size_t focus_index_ = 0;
    
    // Focus navigation
    enum class NavigationDirection : uint8_t {
        Next, Previous, Up, Down, Left, Right
    };
    
public:
    FocusManager() = default;
    
    // Focus management
    void set_focus(std::shared_ptr<Widget> widget);
    void clear_focus();
    std::shared_ptr<Widget> get_focused_widget() const { return focused_widget_; }
    
    // Focus chain management
    void set_focus_chain(const std::vector<std::shared_ptr<Widget>>& chain);
    void add_to_focus_chain(std::shared_ptr<Widget> widget);
    void remove_from_focus_chain(std::shared_ptr<Widget> widget);
    
    // Focus navigation
    bool navigate_focus(NavigationDirection direction);
    bool move_focus_next();
    bool move_focus_previous();
    
    // Focus events
    std::function<void(std::shared_ptr<Widget>)> on_focus_changed;
    
private:
    std::shared_ptr<Widget> find_next_focusable_widget(NavigationDirection direction) const;
    std::shared_ptr<Widget> find_widget_in_direction(NavigationDirection direction, 
                                                  const glm::vec2& from_pos) const;
};

} // namespace Shotcut::UI
```

---

## 5.6 DOCKING AND WINDOW MANAGEMENT

### 5.6.1 Professional Docking System

**Docking Architecture:**
- **Floating Panels:** Draggable, resizable panels
- **Dock Zones:** Multiple docking areas and orientations
- **Tabbed Interfaces:** Multiple panels in single dock zone
- **Auto-Hide:** Professional auto-hide functionality
- **Layout Persistence:** Save and restore panel layouts

**src/ui/docking_system.h:**
```cpp
#pragma once

#include "widget_system.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <optional>

namespace Shotcut::UI {

enum class DockArea : uint8_t {
    Left, Right, Top, Bottom, Center
};

enum class DockOrientation : uint8_t {
    Horizontal, Vertical
};

enum class DockState : uint8_t {
    Floating, Docked, Tabbed, Hidden, AutoHidden
};

struct DockZone {
    Rect bounds;
    DockOrientation orientation;
    std::vector<std::shared_ptr<Widget>> panels;
    size_t active_tab = 0;
    bool auto_hide = false;
    float hide_delay = 0.5f;
    float current_hide_timer = 0.0f;
};

// Dockable panel interface
class IDockable {
public:
    virtual ~IDockable() = default;
    
    virtual std::string dock_title() const = 0;
    virtual std::string dock_id() const = 0;
    virtual DockState dock_state() const = 0;
    virtual void set_dock_state(DockState state) = 0;
    
    virtual std::vector<DockArea> allowed_dock_areas() const = 0;
    virtual bool can_tab_with(std::shared_ptr<IDockable> other) const = 0;
    
    virtual glm::vec2 get_preferred_size() const = 0;
    virtual glm::vec2 get_minimum_size() const = 0;
    virtual glm::vec2 get_maximum_size() const = 0;
};

class DockablePanel : public Widget, public IDockable {
private:
    std::string title_;
    std::string dock_id_;
    DockState state_ = DockState::Floating;
    std::vector<DockArea> allowed_areas_ = {DockArea::Left, DockArea::Right, DockArea::Top, DockArea::Bottom};
    
    glm::vec2 preferred_size_ = glm::vec2(200.0f, 150.0f);
    glm::vec2 minimum_size_ = glm::vec2(100.0f, 75.0f);
    glm::vec2 maximum_size_ = glm::vec2(800.0f, 600.0f);
    
    bool resizable_ = true;
    bool closable_ = true;
    bool floatable_ = true;
    
    // Drag and dock state
    bool dragging_ = false;
    glm::vec2 drag_offset_;
    glm::vec2 dock_preview_position_;
    DockArea preview_dock_area_ = DockArea::Center;
    
public:
    DockablePanel(std::string title, std::string dock_id = "");
    ~DockablePanel() override = default;
    
    // IDockable interface
    std::string dock_title() const override { return title_; }
    std::string dock_id() const override { return dock_id_; }
    DockState dock_state() const override { return state_; }
    void set_dock_state(DockState state) override;
    
    std::vector<DockArea> allowed_dock_areas() const override { return allowed_areas_; }
    bool can_tab_with(std::shared_ptr<IDockable> other) const override { return true; }
    
    glm::vec2 get_preferred_size() const override { return preferred_size_; }
    glm::vec2 get_minimum_size() const override { return minimum_size_; }
    glm::vec2 get_maximum_size() const override { return maximum_size_; }
    
    // Properties
    void set_title(const std::string& title) {
        title_ = title;
        invalidate();
    }
    
    void set_preferred_size(const glm::vec2& size) {
        preferred_size_ = size;
        invalidate_layout();
    }
    
    void set_minimum_size(const glm::vec2& size) {
        minimum_size_ = size;
        invalidate_layout();
    }
    
    void set_maximum_size(const glm::vec2& size) {
        maximum_size_ = size;
        invalidate_layout();
    }
    
    bool resizable() const { return resizable_; }
    void set_resizable(bool resizable) { resizable_ = resizable; }
    
    bool closable() const { return closable_; }
    void set_closable(bool closable) { closable_ = closable; }
    
    bool floatable() const { return floatable_; }
    void set_floatable(bool floatable) { floatable_ = floatable; }
    
protected:
    void render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) override;
    void handle_mouse_event(const MouseEvent& event);
    void start_dragging(const glm::vec2& position);
    void update_dragging(const glm::vec2& position);
    void finish_dragging();
    
    void render_title_bar(VkCommandBuffer command_buffer, const glm::mat4& transform);
    void render_close_button(VkCommandBuffer command_buffer, const glm::mat4& transform);
};

// Docking manager
class DockingManager {
private:
    std::vector<std::unique_ptr<DockZone>> dock_zones_;
    std::unordered_map<std::string, std::shared_ptr<DockablePanel>> panels_;
    std::unordered_map<std::string, DockZone*> panel_to_zone_;
    
    // Layout persistence
    struct LayoutInfo {
        std::vector<std::pair<std::string, DockState>> panel_states;
        std::vector<std::pair<std::string, DockArea>> panel_areas;
        std::vector<std::tuple<std::string, float, float, float, float>> panel_bounds; // id, x, y, w, h
    };
    
    LayoutInfo current_layout_;
    
public:
    DockingManager() = default;
    ~DockingManager() = default;
    
    // Panel management
    void register_panel(std::shared_ptr<DockablePanel> panel);
    void unregister_panel(const std::string& panel_id);
    std::shared_ptr<DockablePanel> get_panel(const std::string& panel_id);
    
    // Docking operations
    bool dock_panel(const std::string& panel_id, DockArea area);
    bool undock_panel(const std::string& panel_id);
    bool tab_panel_with(const std::string& panel_id, const std::string& target_panel_id);
    
    // Zone management
    void create_dock_zone(DockArea area, const Rect& bounds, DockOrientation orientation);
    void update_dock_zone_bounds(DockArea area, const Rect& bounds);
    DockZone* get_dock_zone(DockArea area);
    
    // Auto-hide functionality
    void set_panel_auto_hide(const std::string& panel_id, bool auto_hide, float delay = 0.5f);
    void update_auto_hide_panels(float delta_time);
    
    // Layout persistence
    bool save_layout(const std::string& filename) const;
    bool load_layout(const std::string& filename);
    void reset_to_default_layout();
    
    // Rendering
    void render_dock_overlays(VkCommandBuffer command_buffer, const glm::mat4& transform);
    void render_drag_preview(VkCommandBuffer command_buffer, const glm::mat4& transform);
    
private:
    DockZone* find_best_dock_zone(const glm::vec2& position) const;
    void update_layout_from_current_state();
    void apply_layout(const LayoutInfo& layout);
    
    std::string serialize_layout() const;
    bool deserialize_layout(const std::string& data);
};

// Window management system
class WindowManager {
private:
    struct Window {
        uint32_t id;
        std::string title;
        Rect bounds;
        bool visible = true;
        bool minimized = false;
        bool maximized = false;
        float opacity = 1.0f;
        
        std::shared_ptr<Widget> root_widget;
        DockingManager* docking_manager = nullptr;
    };
    
    std::vector<Window> windows_;
    uint32_t next_window_id_ = 1;
    Window* main_window_ = nullptr;
    Window* active_window_ = nullptr;
    
public:
    WindowManager() = default;
    
    // Window creation and management
    uint32_t create_window(const std::string& title, const Rect& bounds, 
                        std::shared_ptr<Widget> root_widget);
    void destroy_window(uint32_t window_id);
    
    Window* get_window(uint32_t window_id);
    Window* get_main_window() { return main_window_; }
    Window* get_active_window() { return active_window_; }
    
    void set_main_window(uint32_t window_id);
    void set_active_window(uint32_t window_id);
    
    // Window properties
    void set_window_title(uint32_t window_id, const std::string& title);
    void set_window_bounds(uint32_t window_id, const Rect& bounds);
    void set_window_visibility(uint32_t window_id, bool visible);
    void set_window_opacity(uint32_t window_id, float opacity);
    
    void minimize_window(uint32_t window_id);
    void maximize_window(uint32_t window_id);
    void restore_window(uint32_t window_id);
    
    // Window events
    std::function<void(uint32_t)> on_window_created;
    std::function<void(uint32_t)> on_window_destroyed;
    std::function<void(uint32_t)> on_window_activated;
    std::function<void(uint32_t)> on_window_deactivated;
    
    // Rendering
    void render_windows(VkCommandBuffer command_buffer);
    
private:
    Window* find_window_by_id(uint32_t id);
    void update_window_z_order();
};

} // namespace Shotcut::UI
```

---

## 5.7 TIMELINE WIDGET FOUNDATION

### 5.7.1 Professional Timeline Architecture

**Timeline Design Philosophy:**
- **Multi-Track Support:** Video, audio, subtitle, and effect tracks
- **Real-Time Performance:** Smooth scrubbing and playback
- **Zoom and Navigation:** Infinite zoom and pan capabilities
- **Clip Manipulation:** Drag-and-drop, trimming, splitting
- **Professional Features:** JKL navigation, markers, timecode display

**src/ui/timeline_widget.h:**
```cpp
#pragma once

#include "widget_system.h"
#include "docking_system.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <chrono>

namespace Shotcut::UI {

// Time-based coordinate system
using TimePoint = std::chrono::microseconds;
using Duration = std::chrono::microseconds;

struct TimeRange {
    TimePoint start;
    TimePoint end;
    
    Duration duration() const { return end - start; }
    bool contains(TimePoint time) const { return time >= start && time <= end; }
    bool overlaps(const TimeRange& other) const {
        return start < other.end && other.start < end;
    }
    
    TimeRange intersect(const TimeRange& other) const {
        TimePoint new_start = std::max(start, other.start);
        TimePoint new_end = std::min(end, other.end);
        return {new_start, new_end};
    }
};

// Timeline coordinate system
struct TimelineCoords {
    float x, y;          // Pixel coordinates
    TimePoint time;        // Time at this position
    int track_index;       // Track index at this position
    
    TimelineCoords() : x(0), y(0), time(Duration{0}), track_index(-1) {}
    TimelineCoords(float px, float py, TimePoint pt, int ti) 
        : x(px), y(py), time(pt), track_index(ti) {}
};

// Clip types
enum class ClipType : uint8_t {
    Video, Audio, Image, Text, Effect, Transition
};

// Timeline clip
class TimelineClip {
private:
    std::string id_;
    ClipType type_;
    TimeRange time_range_;
    std::string media_path_;
    glm::vec2 position_;
    glm::vec2 size_;
    bool selected_ = false;
    bool muted_ = false;
    bool locked_ = false;
    
    // Clip properties
    float volume_ = 1.0f;
    float opacity_ = 1.0f;
    float speed_ = 1.0f;
    glm::vec2 scale_ = glm::vec2(1.0f);
    float rotation_ = 0.0f;
    
    // Effects
    std::vector<std::string> effect_ids_;
    
public:
    TimelineClip(std::string id, ClipType type, TimeRange range, std::string media_path)
        : id_(std::move(id)), type_(type), time_range_(range), 
          media_path_(std::move(media_path)) {}
    
    // Basic properties
    const std::string& id() const { return id_; }
    ClipType type() const { return type_; }
    const TimeRange& time_range() const { return time_range_; }
    void set_time_range(const TimeRange& range) { time_range_ = range; }
    
    const std::string& media_path() const { return media_path_; }
    
    // Selection and state
    bool selected() const { return selected_; }
    void set_selected(bool selected) { selected_ = selected; }
    
    bool muted() const { return muted_; }
    void set_muted(bool muted) { muted_ = muted; }
    
    bool locked() const { return locked_; }
    void set_locked(bool locked) { locked_ = locked; }
    
    // Visual properties
    const glm::vec2& position() const { return position_; }
    void set_position(const glm::vec2& pos) { position_ = pos; }
    
    const glm::vec2& size() const { return size_; }
    void set_size(const glm::vec2& size) { size_ = size; }
    
    // Audio/video properties
    float volume() const { return volume_; }
    void set_volume(float volume) { volume_ = std::clamp(volume, 0.0f, 2.0f); }
    
    float opacity() const { return opacity_; }
    void set_opacity(float opacity) { opacity_ = std::clamp(opacity, 0.0f, 1.0f); }
    
    float speed() const { return speed_; }
    void set_speed(float speed) { speed_ = std::max(0.1f, speed); }
    
    const glm::vec2& scale() const { return scale_; }
    void set_scale(const glm::vec2& scale) { scale_ = scale; }
    
    float rotation() const { return rotation_; }
    void set_rotation(float rotation) { rotation_ = rotation; }
    
    // Effects
    const std::vector<std::string>& effects() const { return effect_ids_; }
    void add_effect(const std::string& effect_id) {
        effect_ids_.push_back(effect_id);
    }
    void remove_effect(const std::string& effect_id) {
        effect_ids_.erase(std::remove(effect_ids_.begin(), effect_ids_.end(), effect_id), 
                        effect_ids_.end());
    }
    
    // Hit testing
    bool contains_point(const glm::vec2& point) const {
        return point.x >= position_.x && point.x <= position_.x + size_.x &&
               point.y >= position_.y && point.y <= position_.y + size_.y;
    }
    
    bool contains_time(TimePoint time) const {
        return time_range_.contains(time);
    }
};

// Timeline track
class TimelineTrack {
private:
    std::string id_;
    std::string name_;
    ClipType type_;
    int index_;
    float height_;
    bool locked_ = false;
    bool muted_ = false;
    bool soloed_ = false;
    
    std::vector<std::shared_ptr<TimelineClip>> clips_;
    
public:
    TimelineTrack(std::string id, ClipType type, int index, std::string name = "")
        : id_(std::move(id)), type_(type), index_(index), 
          name_(std::move(name)), height_(50.0f) {}
    
    // Basic properties
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    void set_name(std::string name) { name_ = std::move(name); }
    
    ClipType type() const { return type_; }
    int index() const { return index_; }
    void set_index(int index) { index_ = index; }
    
    float height() const { return height_; }
    void set_height(float height) { height_ = std::max(20.0f, height); }
    
    // Track state
    bool locked() const { return locked_; }
    void set_locked(bool locked) { locked_ = locked; }
    
    bool muted() const { return muted_; }
    void set_muted(bool muted) { muted_ = muted; }
    
    bool soloed() const { return soloed_; }
    void set_soloed(bool soloed) { soloed_ = soloed; }
    
    // Clip management
    const std::vector<std::shared_ptr<TimelineClip>>& clips() const { return clips_; }
    
    void add_clip(std::shared_ptr<TimelineClip> clip);
    void remove_clip(const std::string& clip_id);
    std::shared_ptr<TimelineClip> get_clip(const std::string& clip_id);
    std::shared_ptr<TimelineClip> get_clip_at_time(TimePoint time);
    
    std::vector<std::shared_ptr<TimelineClip>> get_clips_in_range(const TimeRange& range);
    
    void move_clip(const std::string& clip_id, TimePoint new_time);
    void trim_clip(const std::string& clip_id, TimePoint new_start, TimePoint new_end);
    void split_clip(const std::string& clip_id, TimePoint split_time);
    
    // Time utilities
    TimePoint get_duration() const;
    TimeRange get_total_range() const;
    
private:
    void sort_clips_by_time();
    std::shared_ptr<TimelineClip> find_clip_before_time(TimePoint time);
    std::shared_ptr<TimelineClip> find_clip_after_time(TimePoint time);
};

// Main timeline widget
class TimelineWidget : public Widget {
private:
    std::vector<std::shared_ptr<TimelineTrack>> tracks_;
    std::vector<std::shared_ptr<TimelineClip>> selected_clips_;
    
    // Timeline properties
    TimeRange visible_range_;
    TimeRange total_range_;
    float pixels_per_second_ = 50.0f;
    float track_height_ = 50.0f;
    
    // Playback state
    TimePoint current_time_;
    bool playing_ = false;
    TimeRange loop_range_;
    
    // Navigation
    TimePoint playhead_position_;
    bool scrubbing_ = false;
    glm::vec2 scroll_offset_;
    
    // Zoom and pan
    float zoom_level_ = 1.0f;
    glm::vec2 pan_offset_;
    
    // Markers
    struct Marker {
        TimePoint time;
        std::string name;
        glm::vec4 color;
        std::string id;
    };
    std::vector<Marker> markers_;
    
    // Rendering
    VkBuffer track_vertex_buffer_;
    VkDeviceMemory track_vertex_memory_;
    VkBuffer clip_vertex_buffer_;
    VkDeviceMemory clip_vertex_memory_;
    VkBuffer playhead_vertex_buffer_;
    VkDeviceMemory playhead_vertex_memory_;
    
    bool gpu_resources_dirty_ = true;
    
public:
    TimelineWidget(std::string id = "timeline");
    ~TimelineWidget() override;
    
    // Track management
    void add_track(std::shared_ptr<TimelineTrack> track);
    void remove_track(const std::string& track_id);
    std::shared_ptr<TimelineTrack> get_track(const std::string& track_id);
    std::shared_ptr<TimelineTrack> get_track_by_index(int index);
    
    const std::vector<std::shared_ptr<TimelineTrack>>& tracks() const { return tracks_; }
    
    // Selection
    const std::vector<std::shared_ptr<TimelineClip>>& selected_clips() const { 
        return selected_clips_; 
    }
    
    void select_clip(const std::string& clip_id, bool multi_select = false);
    void select_clips_in_range(const TimeRange& range);
    void clear_selection();
    
    // Timeline properties
    const TimeRange& visible_range() const { return visible_range_; }
    void set_visible_range(const TimeRange& range);
    
    float pixels_per_second() const { return pixels_per_second_; }
    void set_pixels_per_second(float pps);
    
    TimePoint current_time() const { return current_time_; }
    void set_current_time(TimePoint time);
    
    // Playback control
    bool playing() const { return playing_; }
    void set_playing(bool playing);
    
    void play();
    void pause();
    void stop();
    void play_pause_toggle();
    
    // Navigation
    void seek(TimePoint time);
    void seek_relative(Duration offset);
    
    // JKL navigation
    void jkl_navigation(float speed);
    
    // Zoom and pan
    void zoom_in(const glm::vec2& center);
    void zoom_out(const glm::vec2& center);
    void zoom_to_fit();
    void zoom_to_selection();
    
    void pan(const glm::vec2& delta);
    void center_on_time(TimePoint time);
    
    // Markers
    void add_marker(TimePoint time, const std::string& name, const glm::vec4& color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    void remove_marker(const std::string& marker_id);
    const std::vector<Marker>& markers() const { return markers_; }
    
    // Coordinate conversion
    TimelineCoords screen_to_timeline(const glm::vec2& screen_pos) const;
    glm::vec2 timeline_to_screen(const TimelineCoords& coords) const;
    TimePoint screen_x_to_time(float x) const;
    float time_to_screen_x(TimePoint time) const;
    int screen_y_to_track(float y) const;
    float track_to_screen_y(int track_index) const;
    
    // Layout
    LayoutConstraints calculate_constraints() const override;
    
protected:
    void render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) override;
    void handle_mouse_event(const MouseEvent& event);
    void handle_keyboard_event(const KeyEvent& event);
    
    void on_bounds_changed(const Rect& old_bounds, const Rect& new_bounds) override;
    
private:
    // Rendering
    void render_tracks(VkCommandBuffer command_buffer, const glm::mat4& transform);
    void render_clips(VkCommandBuffer command_buffer, const glm::mat4& transform);
    void render_playhead(VkCommandBuffer command_buffer, const glm::mat4& transform);
    void render_markers(VkCommandBuffer command_buffer, const glm::mat4& transform);
    void render_time_ruler(VkCommandBuffer command_buffer, const glm::mat4& transform);
    
    // GPU resources
    void update_gpu_resources();
    void update_track_vertices();
    void update_clip_vertices();
    void update_playhead_vertices();
    
    // Hit testing
    std::shared_ptr<TimelineClip> hit_test_clip(const glm::vec2& point);
    std::shared_ptr<TimelineTrack> hit_test_track(const glm::vec2& point);
    bool hit_test_playhead(const glm::vec2& point);
    
    // Editing operations
    void start_clip_drag(std::shared_ptr<TimelineClip> clip, const glm::vec2& mouse_pos);
    void update_clip_drag(const glm::vec2& mouse_pos);
    void finish_clip_drag();
    
    void start_clip_trim(std::shared_ptr<TimelineClip> clip, const glm::vec2& mouse_pos, bool trim_start);
    void update_clip_trim(const glm::vec2& mouse_pos);
    void finish_clip_trim();
    
    // Time utilities
    std::string format_time(TimePoint time) const;
    TimePoint parse_time(const std::string& time_str) const;
    
    // Animation
    void animate_playhead_to_time(TimePoint target_time);
    void animate_zoom_to_level(float target_level, const glm::vec2& center);
};

} // namespace Shotcut::UI
```

---

## PHASE 5 IMPLEMENTATION ROADMAP

### 5.8 Month-by-Month Implementation Plan

**Month 4: Core Layout and Widget System**
- Week 1-2: Layout engine implementation
- Week 3-4: Basic widget system with Button and Panel
- Week 5-6: Event handling and input management
- Week 7-8: Focus management and accessibility

**Month 5: Advanced UI Components**
- Week 1-2: Text rendering with FreeType/HarfBuzz
- Week 3-4: Theming system implementation
- Week 5-6: Advanced widgets (Tree view, List view, Property panels)
- Week 7-8: Animation system and transitions

**Month 6: Professional Features**
- Week 1-2: Docking system implementation
- Week 3-4: Window management
- Week 5-6: Timeline widget foundation
- Week 7-8: Timeline clip manipulation and performance optimization

### 5.9 Critical Success Metrics

**Performance Targets:**
- UI frame rate: 60 FPS minimum, 120+ FPS target
- Layout computation: <1ms for complex layouts
- Text rendering: <2ms for 1000+ characters
- Event processing: <0.1ms latency

**Quality Targets:**
- Pixel-perfect rendering accuracy
- Smooth animations with no stuttering
- Professional theme consistency
- Accessibility compliance (WCAG 2.1 AA)

**Professional Features:**
- Keyboard shortcuts for all major functions
- Multi-monitor support
- High DPI rendering (200%+ scaling)
- International text input (IME support)

This Phase 5 implementation guide provides the foundation for building a professional-grade UI framework that rivals commercial video editors, with modern C++20/23 architecture, GPU acceleration, and comprehensive feature set.
