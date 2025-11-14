#pragma once

#include "math_types.h"
#include <deque>
#include <unordered_map>
#include <memory>
#include <functional>
#include <coroutine>
#include <array>
#include <string>
#include <vector>
#include <cstring>

// Forward declarations to avoid circular dependencies
namespace Shotcut::UI {
    class Widget;
}

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
            char text[32];  // Fixed-size array instead of std::string
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
    
    // Default constructor
    InputEvent() : device(InputDevice::Mouse), state(InputState::Released), 
                   timestamp(0.0f), modifiers(0) {
        memset(&mouse, 0, sizeof(mouse));
    }
    
    // Destructor - explicit for union
    ~InputEvent() = default;
};

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
    
    // Key binding system
    void set_key_binding(const std::string& action, uint32_t key_code, uint32_t modifiers = 0);
    void set_action_binding(const std::string& action, std::function<void()> callback);
    bool trigger_action(const std::string& action);
    
private:
    void process_mouse_events();
    void process_keyboard_events();
    void update_mouse_state(const InputEvent& event);
    void update_keyboard_state(const InputEvent& event);
    
    std::unordered_map<std::string, uint32_t> key_bindings_;
    std::unordered_map<std::string, std::function<void()>> action_bindings_;
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
