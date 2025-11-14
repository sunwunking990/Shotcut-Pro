#include "timeline_widget.h"
#include <algorithm>
#include <iostream>
#include <iomanip>

namespace Shotcut::UI {

// TimelineTrack implementation
void TimelineTrack::add_clip(std::shared_ptr<TimelineClip> clip) {
    clips_.push_back(clip);
    sort_clips_by_time();
}

void TimelineTrack::remove_clip(const std::string& clip_id) {
    clips_.erase(
        std::remove_if(clips_.begin(), clips_.end(),
            [&clip_id](const auto& clip) { return clip->id() == clip_id; }),
        clips_.end()
    );
}

std::shared_ptr<TimelineClip> TimelineTrack::get_clip(const std::string& clip_id) {
    auto it = std::find_if(clips_.begin(), clips_.end(),
        [&clip_id](const auto& clip) { return clip->id() == clip_id; });
    return (it != clips_.end()) ? *it : nullptr;
}

std::shared_ptr<TimelineClip> TimelineTrack::get_clip_at_time(TimePoint time) {
    for (const auto& clip : clips_) {
        if (clip->contains_time(time)) {
            return clip;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<TimelineClip>> TimelineTrack::get_clips_in_range(const TimeRange& range) {
    std::vector<std::shared_ptr<TimelineClip>> result;
    for (const auto& clip : clips_) {
        if (clip->time_range().overlaps(range)) {
            result.push_back(clip);
        }
    }
    return result;
}

void TimelineTrack::move_clip(const std::string& clip_id, TimePoint new_time) {
    auto clip = get_clip(clip_id);
    if (clip) {
        auto duration = clip->time_range().duration();
        clip->set_time_range({new_time, new_time + duration});
        sort_clips_by_time();
    }
}

void TimelineTrack::trim_clip(const std::string& clip_id, TimePoint new_start, TimePoint new_end) {
    auto clip = get_clip(clip_id);
    if (clip && new_start < new_end) {
        clip->set_time_range({new_start, new_end});
        sort_clips_by_time();
    }
}

void TimelineTrack::split_clip(const std::string& clip_id, TimePoint split_time) {
    auto clip = get_clip(clip_id);
    if (!clip || !clip->contains_time(split_time)) {
        return;
    }
    
    auto range = clip->time_range();
    if (split_time <= range.start || split_time >= range.end) {
        return;
    }
    
    // Create two new clips
    auto left_clip = std::make_shared<TimelineClip>(
        clip->id() + "_left",
        clip->type(),
        TimeRange{range.start, split_time},
        clip->media_path()
    );
    
    auto right_clip = std::make_shared<TimelineClip>(
        clip->id() + "_right",
        clip->type(),
        TimeRange{split_time, range.end},
        clip->media_path()
    );
    
    // Copy properties
    left_clip->set_volume(clip->volume());
    left_clip->set_opacity(clip->opacity());
    left_clip->set_scale(clip->scale());
    left_clip->set_rotation(clip->rotation());
    
    right_clip->set_volume(clip->volume());
    right_clip->set_opacity(clip->opacity());
    right_clip->set_scale(clip->scale());
    right_clip->set_rotation(clip->rotation());
    
    // Remove original and add new clips
    remove_clip(clip_id);
    add_clip(left_clip);
    add_clip(right_clip);
}

TimePoint TimelineTrack::get_duration() const {
    if (clips_.empty()) {
        return TimePoint(Duration{0});
    }
    
    auto last_clip = clips_.back();
    return last_clip->time_range().end;
}

TimeRange TimelineTrack::get_total_range() const {
    if (clips_.empty()) {
        return {TimePoint(Duration{0}), TimePoint(Duration{0})};
    }
    
    auto first_clip = clips_.front();
    auto last_clip = clips_.back();
    return {first_clip->time_range().start, last_clip->time_range().end};
}

void TimelineTrack::sort_clips_by_time() {
    std::sort(clips_.begin(), clips_.end(),
        [](const auto& a, const auto& b) {
            return a->time_range().start < b->time_range().start;
        });
}

// TimelineWidget implementation
TimelineWidget::TimelineWidget(std::string id) : Widget(std::move(id)) {
    visible_range_ = {TimePoint(Duration{0}), TimePoint(Duration{10'000'000})}; // 10 seconds
    total_range_ = {TimePoint(Duration{0}), TimePoint(Duration{60'000'000})}; // 1 minute
    current_time_ = TimePoint(Duration{0});
    playhead_position_ = TimePoint(Duration{0});
}

TimelineWidget::~TimelineWidget() {
    // Cleanup Vulkan resources
    if (track_vertex_buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkan_context_->device(), track_vertex_buffer_, nullptr);
    }
    if (track_vertex_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(vulkan_context_->device(), track_vertex_memory_, nullptr);
    }
    // ... cleanup other buffers
}

void TimelineWidget::add_track(std::shared_ptr<TimelineTrack> track) {
    tracks_.push_back(track);
    invalidate_layout();
}

void TimelineWidget::remove_track(const std::string& track_id) {
    tracks_.erase(
        std::remove_if(tracks_.begin(), tracks_.end(),
            [&track_id](const auto& track) { return track->id() == track_id; }),
        tracks_.end()
    );
    invalidate_layout();
}

std::shared_ptr<TimelineTrack> TimelineWidget::get_track(const std::string& track_id) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&track_id](const auto& track) { return track->id() == track_id; });
    return (it != tracks_.end()) ? *it : nullptr;
}

std::shared_ptr<TimelineTrack> TimelineWidget::get_track_by_index(int index) {
    if (index >= 0 && index < static_cast<int>(tracks_.size())) {
        return tracks_[index];
    }
    return nullptr;
}

void TimelineWidget::select_clip(const std::string& clip_id, bool multi_select) {
    if (!multi_select) {
        clear_selection();
    }
    
    for (const auto& track : tracks_) {
        auto clip = track->get_clip(clip_id);
        if (clip) {
            clip->set_selected(true);
            selected_clips_.push_back(clip);
            break;
        }
    }
}

void TimelineWidget::select_clips_in_range(const TimeRange& range) {
    clear_selection();
    
    for (const auto& track : tracks_) {
        auto clips = track->get_clips_in_range(range);
        for (const auto& clip : clips) {
            clip->set_selected(true);
            selected_clips_.push_back(clip);
        }
    }
}

void TimelineWidget::clear_selection() {
    for (const auto& clip : selected_clips_) {
        clip->set_selected(false);
    }
    selected_clips_.clear();
}

void TimelineWidget::set_visible_range(const TimeRange& range) {
    visible_range_ = range;
    invalidate();
}

void TimelineWidget::set_pixels_per_second(float pps) {
    pixels_per_second_ = std::max(1.0f, pps);
    invalidate();
}

void TimelineWidget::set_current_time(TimePoint time) {
    current_time_ = time;
    playhead_position_ = time;
    invalidate();
}

void TimelineWidget::set_playing(bool playing) {
    playing_ = playing;
    std::cout << (playing ? "▶️ Timeline playing" : "⏸️ Timeline paused") << std::endl;
}

void TimelineWidget::play() {
    set_playing(true);
}

void TimelineWidget::pause() {
    set_playing(false);
}

void TimelineWidget::stop() {
    set_playing(false);
    set_current_time(TimePoint(Duration{0}));
    std::cout << "⏹️ Timeline stopped" << std::endl;
}

void TimelineWidget::play_pause_toggle() {
    if (playing_) {
        pause();
    } else {
        play();
    }
}

void TimelineWidget::seek(TimePoint time) {
    set_current_time(time);
    std::cout << "⏩ Seeked to: " << format_time(time) << std::endl;
}

void TimelineWidget::seek_relative(Duration offset) {
    seek(current_time_ + offset);
}

void TimelineWidget::jkl_navigation(float speed) {
    std::cout << "🎮 JKL navigation speed: " << speed << "x" << std::endl;
    // TODO: Implement JKL playback control
}

void TimelineWidget::zoom_in(const glm::vec2& center) {
    zoom_level_ *= 1.5f;
    pixels_per_second_ *= 1.5f;
    std::cout << "🔍 Zoomed in, level: " << zoom_level_ << "x" << std::endl;
    invalidate();
}

void TimelineWidget::zoom_out(const glm::vec2& center) {
    zoom_level_ /= 1.5f;
    pixels_per_second_ /= 1.5f;
    std::cout << "🔍 Zoomed out, level: " << zoom_level_ << "x" << std::endl;
    invalidate();
}

void TimelineWidget::zoom_to_fit() {
    zoom_level_ = 1.0f;
    pixels_per_second_ = 50.0f;
    std::cout << "🔍 Zoomed to fit" << std::endl;
    invalidate();
}

void TimelineWidget::zoom_to_selection() {
    if (selected_clips_.empty()) {
        zoom_to_fit();
        return;
    }
    
    // Calculate selection range
    TimeRange selection_range = selected_clips_[0]->time_range();
    for (const auto& clip : selected_clips_) {
        auto range = clip->time_range();
        selection_range.start = std::min(selection_range.start, range.start);
        selection_range.end = std::max(selection_range.end, range.end);
    }
    
    set_visible_range(selection_range);
    std::cout << "🎯 Zoomed to selection" << std::endl;
}

void TimelineWidget::pan(const glm::vec2& delta) {
    pan_offset_ += delta;
    invalidate();
}

void TimelineWidget::center_on_time(TimePoint time) {
    // Calculate center time for visible range
    auto duration = visible_range_.duration();
    auto center_time = time - duration / 2;
    set_visible_range({center_time, center_time + duration});
}

void TimelineWidget::add_marker(TimePoint time, const std::string& name, const glm::vec4& color) {
    static int marker_id = 0;
    markers_.push_back({
        time,
        name,
        color,
        "marker_" + std::to_string(marker_id++)
    });
    std::cout << "📍 Added marker: " << name << " at " << format_time(time) << std::endl;
}

void TimelineWidget::remove_marker(const std::string& marker_id) {
    markers_.erase(
        std::remove_if(markers_.begin(), markers_.end(),
            [&marker_id](const auto& marker) { return marker.id == marker_id; }),
        markers_.end()
    );
}

TimelineCoords TimelineWidget::screen_to_timeline(const glm::vec2& screen_pos) const {
    TimelineCoords coords;
    coords.x = screen_pos.x;
    coords.y = screen_pos.y;
    coords.time = screen_x_to_time(screen_pos.x);
    coords.track_index = screen_y_to_track(screen_pos.y);
    return coords;
}

glm::vec2 TimelineWidget::timeline_to_screen(const TimelineCoords& coords) const {
    return glm::vec2(time_to_screen_x(coords.time), track_to_screen_y(coords.track_index));
}

TimePoint TimelineWidget::screen_x_to_time(float x) const {
    auto visible_duration = visible_range_.duration();
    auto offset = (x / get_bounds().width) * visible_duration;
    return visible_range_.start + std::chrono::duration_cast<Duration>(offset);
}

float TimelineWidget::time_to_screen_x(TimePoint time) const {
    auto visible_duration = visible_range_.duration();
    auto offset = time - visible_range_.start;
    return (static_cast<float>(offset.count()) / static_cast<float>(visible_duration.count())) * get_bounds().width;
}

int TimelineWidget::screen_y_to_track(float y) const {
    float track_y = 50.0f; // Time ruler height
    int track_index = static_cast<int>((y - track_y) / track_height_);
    return std::clamp(track_index, 0, static_cast<int>(tracks_.size()) - 1);
}

float TimelineWidget::track_to_screen_y(int track_index) const {
    return 50.0f + track_index * track_height_; // 50px for time ruler
}

LayoutConstraints TimelineWidget::calculate_constraints() const {
    LayoutConstraints constraints;
    constraints.min_width = 400.0f;
    constraints.min_height = 200.0f;
    constraints.max_width = std::nullopt; // Flexible width
    constraints.max_height = std::nullopt; // Flexible height
    return constraints;
}

void TimelineWidget::render_content(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    std::cout << "🎨 Rendering timeline widget..." << std::endl;
    
    // In a real implementation, this would:
    // 1. Update GPU vertex buffers
    // 2. Bind pipeline and render tracks
    // 3. Render clips on tracks
    // 4. Render playhead
    // 5. Render markers
    // 6. Render time ruler
    
    render_tracks(command_buffer, transform);
    render_clips(command_buffer, transform);
    render_playhead(command_buffer, transform);
    render_markers(command_buffer, transform);
    render_time_ruler(command_buffer, transform);
}

void TimelineWidget::handle_mouse_event(const MouseEvent& event) {
    std::cout << "🖱️ Timeline mouse event at (" << event.position.x << ", " << event.position.y << ")" << std::endl;
    
    auto coords = screen_to_timeline(event.position);
    auto track = get_track_by_index(coords.track_index);
    
    // Handle mouse down event through the event system
    add_event_handler<EventType::MouseDown>([this](const Event& event) {
        if (const auto* mouse_event = std::get_if<MouseEvent>(&event.data)) {
            if (mouse_event->button == 1) { // Left click
                auto coords = screen_to_timeline(mouse_event->position);
                auto track = get_track_by_index(coords.track_index);
                
                if (track) {
                    auto clip = track->get_clip_at_time(coords.time);
                    if (clip) {
                        select_clip(clip->id(), mouse_event->modifiers & 1); // Shift for multi-select
                        std::cout << "📌 Selected clip: " << clip->id() << std::endl;
                    } else {
                        clear_selection();
                    }
                }
            }
        }
        return true;
    });
}

void TimelineWidget::handle_keyboard_event(const KeyEvent& event) {
    std::cout << "⌨️ Timeline key: " << event.key_code << std::endl;
    
    // Handle key down event through event system
    add_event_handler<EventType::KeyDown>([this](const Event& event) {
        if (const auto* key_event = std::get_if<KeyEvent>(&event.data)) {
            switch (key_event->key_code) {
                case 32: // Space
                    play_pause_toggle();
                    break;
                case 74: // J
                    jkl_navigation(-1.0f);
                    break;
                case 75: // K
                    pause();
                    break;
                case 76: // L
                    jkl_navigation(1.0f);
                    break;
                case 46: // Delete
                    if (!selected_clips_.empty()) {
                        std::cout << "🗑️ Deleting " << selected_clips_.size() << " selected clips" << std::endl;
                        for (const auto& clip : selected_clips_) {
                            // Find and remove clip from track
                            for (const auto& track : tracks_) {
                                track->remove_clip(clip->id());
                            }
                        }
                        clear_selection();
                    }
                    break;
            }
        }
        return true;
    });
}

void TimelineWidget::on_bounds_changed(const Rect& old_bounds, const Rect& new_bounds) {
    Widget::on_bounds_changed(old_bounds, new_bounds);
    gpu_resources_dirty_ = true;
}

void TimelineWidget::render_tracks(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    std::cout << "  📊 Rendering " << tracks_.size() << " tracks" << std::endl;
}

void TimelineWidget::render_clips(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    int total_clips = 0;
    for (const auto& track : tracks_) {
        total_clips += track->clips().size();
    }
    std::cout << "  🎬 Rendering " << total_clips << " clips" << std::endl;
}

void TimelineWidget::render_playhead(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    std::cout << "  ⏱️ Rendering playhead at " << format_time(playhead_position_) << std::endl;
}

void TimelineWidget::render_markers(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    std::cout << "  📍 Rendering " << markers_.size() << " markers" << std::endl;
}

void TimelineWidget::render_time_ruler(VkCommandBuffer command_buffer, const glm::mat4& transform) {
    std::cout << "  📏 Rendering time ruler" << std::endl;
}

std::string TimelineWidget::format_time(TimePoint time) const {
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(time).count();
    auto hours = total_seconds / 3600;
    auto minutes = (total_seconds % 3600) / 60;
    auto seconds = total_seconds % 60;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setw(2) << minutes << ":"
        << std::setw(2) << seconds;
    
    return oss.str();
}

TimePoint TimelineWidget::parse_time(const std::string& time_str) const {
    // TODO: Implement time parsing
    return TimePoint(Duration{0});
}

} // namespace Shotcut::UI
