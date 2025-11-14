#include "widget_system.h"
#include "theme_system.h"
#include <vulkan/vulkan.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace Shotcut::UI {

// Widget base implementation
void Widget::set_state(State new_state) noexcept {
    State old_state = state_;
    state_ = new_state;
    
    if (old_state != new_state) {
        on_state_changed(old_state, new_state);
        invalidate();
    }
}

LayoutConstraints Widget::calculate_constraints() const {
    // Default constraints - can be overridden by derived classes
    LayoutConstraints constraints;
    auto bounds = get_bounds();
    
    constraints.min_width = constraints.max_width = bounds.width;
    constraints.min_height = constraints.max_height = bounds.height;
    constraints.preferred_width = bounds.width;
    constraints.preferred_height = bounds.height;
    
    return constraints;
}

void Widget::apply_constraints(const LayoutConstraints& constraints) {
    auto bounds = get_bounds();
    
    if (constraints.min_width) {
        bounds.width = std::max(bounds.width, *constraints.min_width);
    }
    if (constraints.max_width) {
        bounds.width = std::min(bounds.width, *constraints.max_width);
    }
    if (constraints.min_height) {
        bounds.height = std::max(bounds.height, *constraints.min_height);
    }
    if (constraints.max_height) {
        bounds.height = std::min(bounds.height, *constraints.max_height);
    }
    
    set_bounds(bounds);
}

void Widget::add_child(std::shared_ptr<LayoutElement> child) {
    // Base Widget doesn't support children by default
    // This should be overridden by container widgets
}

void Widget::remove_child(std::shared_ptr<LayoutElement> child) {
    // Base Widget doesn't support children by default
}

std::span<std::shared_ptr<LayoutElement>> Widget::get_children() {
    // Return empty span for non-container widgets
    static std::vector<std::shared_ptr<LayoutElement>> empty_children;
    return empty_children;
}

void Widget::set_flex_properties(const FlexProperties& props) {
    // Store flex properties for derived classes
    // Implementation depends on specific widget type
}

const FlexProperties& Widget::get_flex_properties() const {
    // Return default flex properties
    static FlexProperties default_props;
    return default_props;
}

void Widget::compute_layout(float available_width, float available_height) {
    // Basic layout computation for single widgets
    auto constraints = calculate_constraints();
    apply_constraints(constraints);
    
    if (children_.size() > 0) {
        // If this widget has children, compute their layouts
        for (auto& child : children_) {
            child->compute_layout(available_width, available_height);
        }
    }
}

void Widget::invalidate_layout() {
    needs_layout_ = true;
    
    // Propagate layout invalidation to parent
    if (auto parent_widget = std::dynamic_pointer_cast<Widget>(parent_)) {
        parent_widget->invalidate_layout();
    }
}

bool Widget::is_layout_valid() const {
    return needs_layout_;
}

void Widget::handle_event(const Event& event) {
    on_event(event);
    
    // Propagate event to children if needed
    propagate_event_to_children(event);
}

bool Widget::handles_event(EventType type) const {
    auto it = event_handlers_.find(type);
    return it != event_handlers_.end() && !it->second.empty();
}

void Widget::render(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    if (visibility_ == Visibility::Hidden || visibility_ == Visibility::Collapsed) {
        return;
    }
    
    // Calculate widget transform
    glm::mat4 widget_transform = get_transform() * transform;
    
    // Render widget content
    render_content(command_buffer, widget_transform);
    
    // Render children
    render_children(command_buffer, widget_transform);
}

void Widget::render_children(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    for (auto& child : children_) {
        if (auto child_widget = std::dynamic_pointer_cast<Widget>(child)) {
            child_widget->render(command_buffer, transform);
        }
    }
}

std::optional<std::shared_ptr<StyleProperty>> Widget::get_style(const std::string& property) const {
    // Check custom styles first
    auto it = custom_styles_.find(property);
    if (it != custom_styles_.end()) {
        return it->second;
    }
    
    // Fall back to theme
    if (theme_) {
        auto theme_prop = theme_->get_property(property);
        if (theme_prop) {
            return std::make_shared<StyleProperty>(*theme_prop);
        }
    }
    
    return std::nullopt;
}

std::shared_ptr<Widget> Widget::hit_test(const glm::vec2& point) const {
    // Check if point is within widget bounds
    if (!contains_point(point)) {
        return nullptr;
    }
    
    // Test children first (they're on top)
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (auto child_widget = std::dynamic_pointer_cast<Widget>(*it)) {
            auto hit = child_widget->hit_test(point);
            if (hit) {
                return hit;
            }
        }
    }
    
    // This widget is hit
    return std::const_pointer_cast<Widget>(shared_from_this());
}

bool Widget::contains_point(const glm::vec2& point) const {
    return bounds_.contains(point);
}

glm::mat4 Widget::get_transform() const {
    // Create transform matrix from bounds and opacity
    glm::mat4 transform = glm::mat4(1.0f);
    
    // Translate to widget position
    transform = glm::translate(transform, glm::vec3(bounds_.x, bounds_.y, 0.0f));
    
    return transform;
}

void Widget::propagate_event_to_children(const Event& event) {
    for (auto& child : children_) {
        if (auto child_widget = std::dynamic_pointer_cast<Widget>(child)) {
            child_widget->handle_event(event);
        }
    }
}

std::shared_ptr<Widget> Widget::find_child_at_point(const glm::vec2& point) const {
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        if (auto child_widget = std::dynamic_pointer_cast<Widget>(*it)) {
            if (child_widget->contains_point(point)) {
                return child_widget;
            }
        }
    }
    return nullptr;
}

// Button implementation
void Button::on_state_changed(State old_state, State new_state) {
    // Change appearance based on state
    invalidate();
}

void Button::render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    // TODO: Implement button rendering with Vulkan
    // This would involve:
    // 1. Setting up vertices for button background
    // 2. Creating appropriate colors based on state
    // 3. Rendering text on top of button
    
    // For now, this is a placeholder
    // Actual implementation would use Vulkan command buffers
}

// Panel implementation
void Panel::render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    // TODO: Implement panel rendering with Vulkan
    // This would involve:
    // 1. Rendering background with border radius
    // 2. Rendering border if needed
    // 3. Handling scrollbars if scrollable
    
    // For now, this is a placeholder
}

void Panel::handle_scroll_event(const MouseEvent& event) {
    if (!scrollable_) return;
    
    // Update scroll offset based on wheel delta
    scroll_offset_.y += event.delta.y;
    
    // Clamp scroll offset
    // TODO: Calculate proper bounds based on content size
    scroll_offset_.y = std::clamp(scroll_offset_.y, 0.0f, 1000.0f);
    
    invalidate();
}

} // namespace Shotcut::UI
