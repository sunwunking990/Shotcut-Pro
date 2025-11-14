#include "input_manager.h"
#include <algorithm>
#include <chrono>
#include <limits>

namespace Shotcut::UI {

// Input Manager Implementation
void InputManager::update(float delta_time) {
    // Process queued events
    process_mouse_events();
    process_keyboard_events();
    
    // Clear processed events
    event_queue_.clear();
}

void InputManager::queue_event(InputEvent event) {
    event.timestamp = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count()) / 1000.0f;
    
    event_queue_.push_back(event);
}

uint32_t InputManager::add_event_filter(std::function<bool(const InputEvent&)> filter) {
    static uint32_t next_id = 1;
    uint32_t id = next_id++;
    event_filters_[id] = std::move(filter);
    return id;
}

void InputManager::remove_event_filter(uint32_t filter_id) {
    event_filters_.erase(filter_id);
}

uint32_t InputManager::add_event_handler(std::function<void(const InputEvent&)> handler) {
    static uint32_t next_id = 1;
    uint32_t id = next_id++;
    event_handlers_[id] = std::move(handler);
    return id;
}

void InputManager::remove_event_handler(uint32_t handler_id) {
    event_handlers_.erase(handler_id);
}

void InputManager::process_mouse_events() {
    for (auto& event : event_queue_) {
        if (event.device != InputDevice::Mouse) continue;
        
        update_mouse_state(event);
        
        // Call handlers
        for (const auto& [id, handler] : event_handlers_) {
            handler(event);
        }
    }
}

void InputManager::process_keyboard_events() {
    for (auto& event : event_queue_) {
        if (event.device != InputDevice::Keyboard) continue;
        
        update_keyboard_state(event);
        
        // Call handlers
        for (const auto& [id, handler] : event_handlers_) {
            handler(event);
        }
    }
}

void InputManager::update_mouse_state(const InputEvent& event) {
    mouse_state_.last_position = mouse_state_.position;
    mouse_state_.last_buttons = mouse_state_.buttons;
    
    mouse_state_.position = event.mouse.position;
}

void InputManager::update_keyboard_state(const InputEvent& event) {
    if (event.keyboard.key_code >= 256) return;
    
    keyboard_state_.modifiers = event.modifiers;
}

void InputManager::set_key_binding(const std::string& action, uint32_t key_code, uint32_t modifiers) {
    uint32_t binding_key = (modifiers << 8) | key_code;
    key_bindings_[action] = binding_key;
}

void InputManager::set_action_binding(const std::string& action, std::function<void()> callback) {
    action_bindings_[action] = std::move(callback);
}

bool InputManager::trigger_action(const std::string& action) {
    auto it = action_bindings_.find(action);
    if (it != action_bindings_.end()) {
        it->second();
        return true;
    }
    return false;
}

// Focus Manager Implementation
void FocusManager::set_focus(std::shared_ptr<Widget> widget) {
    if (focused_widget_ == widget) return;
    
    focused_widget_ = widget;
}

void FocusManager::clear_focus() {
    set_focus(nullptr);
}

void FocusManager::set_focus_chain(const std::vector<std::shared_ptr<Widget>>& chain) {
    focus_chain_ = chain;
    focus_index_ = 0;
}

void FocusManager::add_to_focus_chain(std::shared_ptr<Widget> widget) {
    focus_chain_.push_back(widget);
}

void FocusManager::remove_from_focus_chain(std::shared_ptr<Widget> widget) {
    auto it = std::find(focus_chain_.begin(), focus_chain_.end(), widget);
    if (it != focus_chain_.end()) {
        focus_chain_.erase(it);
    }
}

bool FocusManager::navigate_focus(NavigationDirection direction) {
    if (focus_chain_.empty()) return false;
    return true;
}

bool FocusManager::move_focus_next() {
    return navigate_focus(NavigationDirection::Next);
}

bool FocusManager::move_focus_previous() {
    return navigate_focus(NavigationDirection::Previous);
}

} // namespace Shotcut::UI
