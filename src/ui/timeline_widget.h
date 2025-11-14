#pragma once

#include "widget_system.h"
#include "../rendering/vulkan_context.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <chrono>

namespace Shotcut {
namespace UI {

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
    
    // Vulkan context reference
    VulkanContext* vulkan_context_ = nullptr;
    
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

} // namespace UI
} // namespace Shotcut
