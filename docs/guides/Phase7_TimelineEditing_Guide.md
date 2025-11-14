# Phase 7: Timeline & Editing Features Implementation Guide - Shotcut C++ Transformation

**Document Created:** November 13, 2025  
**Implementation Focus:** Professional Timeline System and Editing Capabilities  
**Duration:** Months 11-14 (Timeline Editing Phase)  
**Prerequisites:** Phase 6 completed (Video Processing Pipeline)

---

## PHASE 7 OVERVIEW

Phase 7 focuses on creating a professional-grade timeline and editing system that rivals commercial editors like Adobe Premiere Pro and DaVinci Resolve. This phase implements the core editing functionality including timeline data structures, clip management, track systems, and professional editing operations with real-time performance and precision.

**Critical Success Factors:**
- **Professional Timeline Architecture:** Multi-track support with frame-accurate editing
- **Real-Time Performance:** Smooth editing even with complex timelines and 4K footage
- **Modern C++20/23 Integration:** ECS architecture with optimal performance
- **Professional Editing Tools:** Cut, trim, split, ripple edits, slip/slide
- **Non-Destructive Editing:** Preserve original media with effect stacks

---

## 7.1 TIMELINE DATA STRUCTURE IMPLEMENTATION

### 7.1.1 Entity Component System Timeline Architecture

**Timeline ECS Design Philosophy:**
- **Performance-First:** Optimal cache locality for complex timelines
- **Scalability:** Handle thousands of clips without performance degradation
- **Flexibility:** Easy to add new features and clip types
- **Memory Efficiency:** Compact storage with minimal overhead

**src/timeline/timeline_ecs.h:**
```cpp
#pragma once

#include <entt/entt.hpp>
#include <chrono>
#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <mutex>

namespace Shotcut::Timeline {

using TimePoint = std::chrono::microseconds;
using Duration = std::chrono::microseconds;

// Timeline components for ECS architecture

// Basic timeline entity component
struct TimelineEntity {
    entt::entity entity;
    std::string id;
    std::string name;
    TimelineEntityType type;
    TimePoint created_at;
    bool is_valid = true;
    
    TimelineEntity(entt::entity e, std::string entity_id, std::string entity_name, TimelineEntityType t)
        : entity(e), id(std::move(entity_id)), name(std::move(entity_name)), type(t),
          created_at(std::chrono::high_resolution_clock::now()) {}
};

// Time range component
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
    
    TimeRange union_with(const TimeRange& other) const {
        TimePoint new_start = std::min(start, other.start);
        TimePoint new_end = std::max(end, other.end);
        return {new_start, new_end};
    }
};

// Clip component
struct Clip {
    std::string media_path;
    std::string source_id;
    TimeRange source_range;  // Range in source media
    TimeRange timeline_range; // Range on timeline
    float speed = 1.0f;
    bool reversed = false;
    bool muted = false;
    bool locked = false;
    float volume = 1.0f;
    float opacity = 1.0f;
    
    // Transform
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec2 scale = glm::vec2(1.0f);
    float rotation = 0.0f;
    glm::vec2 anchor = glm::vec2(0.5f);
    
    // Blending
    enum class BlendMode : uint8_t {
        Normal, Multiply, Screen, Overlay, Darken, Lighten,
        ColorDodge, ColorBurn, HardLight, SoftLight, Difference, Exclusion
    } blend_mode = BlendMode::Normal;
    
    // Effects
    std::vector<std::string> effect_ids;
    std::unordered_map<std::string, float> effect_parameters;
    
    // Keyframes
    struct Keyframe {
        TimePoint time;
        std::string parameter;
        std::variant<float, glm::vec2, glm::vec3, glm::vec4> value;
        std::string interpolation; // linear, ease_in, ease_out, bezier
    };
    std::vector<Keyframe> keyframes;
};

// Track component
struct Track {
    enum class Type : uint8_t {
        Video, Audio, Subtitle, Effect, Marker
    } type = Type::Video;
    
    std::string name;
    int index = 0;
    float height = 50.0f;
    bool locked = false;
    bool muted = false;
    bool soloed = false;
    
    // Track-specific properties
    struct VideoProperties {
        glm::vec4 background_color = glm::vec4(0.0f);
        bool show_waveform = false;
        bool show_thumbnails = true;
        float thumbnail_quality = 0.5f;
    } video_props;
    
    struct AudioProperties {
        float pan = 0.0f;
        float gain = 0.0f;
        bool show_waveform = true;
        float waveform_scale = 1.0f;
        enum class Metering : uint8_t { Peak, RMS, LUFS } metering = Metering::Peak;
    } audio_props;
    
    struct SubtitleProperties {
        std::string font_family = "Arial";
        float font_size = 24.0f;
        glm::vec4 text_color = glm::vec4(1.0f);
        glm::vec4 background_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.8f);
        float outline_width = 2.0f;
        glm::vec4 outline_color = glm::vec4(0.0f);
    } subtitle_props;
};

// Marker component
struct Marker {
    std::string name;
    std::string description;
    TimePoint time;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    std::string category; // "chapter", "scene", "note", "error", etc.
    bool is_duration_marker = false;
    Duration duration = Duration{0};
    
    // Marker-specific data
    std::unordered_map<std::string, std::string> metadata;
    std::vector<std::string> tags;
};

// Transition component
struct Transition {
    enum class Type : uint8_t {
        None, CrossDissolve, DipToColor, Fade, Push, Slide, Wipe,
        Zoom, Pan, Spin, Blinds, Iris, PageTurn, Cube, Flip
    } type = Type::None;
    
    TimeRange timeline_range;
    Duration duration = Duration{0};
    
    // Transition parameters
    std::unordered_map<std::string, std::variant<float, glm::vec4, std::string>> parameters;
    
    // Connected clips
    entt::entity from_clip = entt::null;
    entt::entity to_clip = entt::null;
    
    // Transition preview
    bool preview_enabled = true;
    float preview_quality = 0.5f;
};

// Timeline registry and manager
class TimelineManager {
private:
    entt::registry registry_;
    std::unordered_map<std::string, entt::entity> entity_map_;
    std::mutex registry_mutex_;
    
    // Timeline properties
    TimePoint duration_ = Duration{0};
    TimePoint current_time_ = Duration{0};
    AVRational time_base_ = {1, 1000000}; // microseconds
    
    // Track management
    std::vector<entt::entity> tracks_;
    std::unordered_map<int, entt::entity> track_index_map_;
    
    // Selection
    std::vector<entt::entity> selected_entities_;
    std::unordered_set<entt::entity> selection_set_;
    
    // History/undo system
    struct HistoryEntry {
        std::string description;
        TimePoint timestamp;
        std::function<void()> undo_action;
        std::function<void()> redo_action;
    };
    
    std::vector<HistoryEntry> history_;
    size_t history_index_ = 0;
    size_t max_history_size_ = 100;
    
    // Performance optimization
    bool needs_rebuild_ = true;
    std::vector<entt::entity> entities_in_time_range_;
    TimePoint cached_time_range_start_ = Duration{0};
    TimePoint cached_time_range_end_ = Duration{0};
    
public:
    TimelineManager() = default;
    ~TimelineManager() = default;
    
    // Entity management
    entt::entity create_entity(const std::string& id, const std::string& name, TimelineEntityType type);
    void destroy_entity(entt::entity entity);
    entt::entity get_entity(const std::string& id) const;
    bool has_entity(const std::string& id) const;
    
    // Component management
    template<typename T, typename... Args>
    T& add_component(entt::entity entity, Args&&... args) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        needs_rebuild_ = true;
        return registry_.emplace<T>(entity, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& get_component(entt::entity entity) {
        return registry_.get<T>(entity);
    }
    
    template<typename T>
    const T& get_component(entt::entity entity) const {
        return registry_.get<T>(entity);
    }
    
    template<typename T>
    bool has_component(entt::entity entity) const {
        return registry_.all_of<T>(entity);
    }
    
    template<typename T>
    void remove_component(entt::entity entity) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        needs_rebuild_ = true;
        registry_.erase<T>(entity);
    }
    
    // Track management
    entt::entity create_track(const std::string& name, Track::Type type, int index);
    void remove_track(entt::entity track);
    const std::vector<entt::entity>& get_tracks() const { return tracks_; }
    entt::entity get_track_by_index(int index) const;
    
    // Clip management
    entt::entity create_clip(const std::string& id, const std::string& media_path,
                          const TimeRange& source_range, const TimeRange& timeline_range);
    std::vector<entt::entity> get_clips_in_range(const TimeRange& range) const;
    std::vector<entt::entity> get_clips_on_track(entt::entity track) const;
    
    // Selection management
    void select_entity(entt::entity entity, bool multi_select = false);
    void select_entities_in_range(const TimeRange& range, bool multi_select = false);
    void select_all();
    void clear_selection();
    void invert_selection();
    
    const std::vector<entt::entity>& get_selected_entities() const { return selected_entities_; }
    bool is_selected(entt::entity entity) const;
    
    // Timeline operations
    void set_current_time(TimePoint time);
    TimePoint get_current_time() const { return current_time_; }
    TimePoint get_duration() const;
    
    // History/undo
    void begin_history_batch(const std::string& description);
    void end_history_batch();
    void add_history_entry(const std::string& description, 
                         std::function<void()> undo_action, 
                         std::function<void()> redo_action);
    bool can_undo() const;
    bool can_redo() const;
    void undo();
    void redo();
    void clear_history();
    
    // Performance optimization
    void rebuild_caches();
    const std::vector<entt::entity>& get_entities_in_time_range(const TimeRange& range);
    
    // Queries
    template<typename... Components>
    auto view() {
        return registry_.view<Components...>();
    }
    
    template<typename... Components>
    auto view() const {
        return registry_.view<Components...>();
    }
    
    // Validation
    bool validate_timeline() const;
    std::vector<std::string> get_validation_errors() const;
    
private:
    void update_duration();
    void update_selection_cache();
    
    std::function<void()> create_undo_function(const std::string& entity_id, 
                                         TimelineEntityType type);
    std::function<void()> create_redo_function(const std::string& entity_id, 
                                         TimelineEntityType type);
};

} // namespace Shotcut::Timeline
```

### 7.1.2 Timeline Serialization and Persistence

**Timeline Serialization Philosophy:**
- **Efficient Storage:** Binary format with compression for large projects
- **Version Compatibility:** Forward and backward compatibility
- **Integrity Checking:** Checksums and validation for project files
- **Incremental Saves:** Fast saves for large projects

**src/timeline/timeline_serialization.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <zlib.h>

namespace Shotcut::Timeline {

enum class SerializationFormat : uint8_t {
    Binary,     // Custom binary format (fast, compact)
    JSON,       // Human-readable format (debug, interchange)
    XML,        // Legacy compatibility
    Compressed  // Compressed binary format
};

struct SerializationHeader {
    char magic[4] = {'S', 'C', 'T', 'L'}; // Shotcut Timeline
    uint16_t version_major = 1;
    uint16_t version_minor = 0;
    uint32_t flags = 0;
    uint64_t timestamp = 0;
    uint32_t checksum = 0;
    uint64_t uncompressed_size = 0;
    uint64_t compressed_size = 0;
    SerializationFormat format = SerializationFormat::Binary;
};

class TimelineSerializer {
private:
    SerializationHeader header_;
    std::vector<uint8_t> buffer_;
    size_t write_position_ = 0;
    
    // Compression
    bool use_compression_ = false;
    int compression_level_ = 6;
    
public:
    TimelineSerializer();
    ~TimelineSerializer() = default;
    
    // Serialization control
    void set_format(SerializationFormat format);
    void set_compression(bool enabled, int level = 6);
    
    // Header operations
    void write_header();
    bool read_header(std::istream& stream);
    
    // Basic type writing
    template<typename T>
    void write(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        write_bytes(&value, sizeof(T));
    }
    
    void write_bytes(const void* data, size_t size);
    void write_string(const std::string& str);
    void write_vector(const std::vector<uint8_t>& vec);
    
    // Complex type writing
    void write_timepoint(TimePoint time);
    void write_duration(Duration duration);
    void write_vec2(const glm::vec2& vec);
    void write_vec3(const glm::vec3& vec);
    void write_vec4(const glm::vec4& vec);
    
    // Entity serialization
    void write_entity(entt::entity entity, const TimelineEntity& info);
    void write_clip(const Clip& clip);
    void write_track(const Track& track);
    void write_marker(const Marker& marker);
    void write_transition(const Transition& transition);
    
    // Timeline serialization
    void serialize_timeline(const TimelineManager& timeline);
    
    // Output
    bool save_to_file(const std::string& filename);
    bool save_to_stream(std::ostream& stream);
    std::vector<uint8_t> get_buffer() const { return buffer_; }
    
    // Validation
    uint32_t calculate_checksum() const;
    bool validate_checksum() const;
    
private:
    void finalize_header();
    void compress_buffer();
    void decompress_buffer(const std::vector<uint8_t>& compressed);
};

class TimelineDeserializer {
private:
    SerializationHeader header_;
    std::vector<uint8_t> buffer_;
    std::vector<uint8_t> decompressed_buffer_;
    size_t read_position_ = 0;
    bool use_decompressed_ = false;
    
public:
    TimelineDeserializer();
    ~TimelineDeserializer() = default;
    
    // Loading
    bool load_from_file(const std::string& filename);
    bool load_from_stream(std::istream& stream);
    bool load_from_buffer(const std::vector<uint8_t>& buffer);
    
    // Header access
    const SerializationHeader& get_header() const { return header_; }
    bool is_valid_format() const;
    bool is_compatible_version() const;
    
    // Basic type reading
    template<typename T>
    T read() {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        T value;
        read_bytes(&value, sizeof(T));
        return value;
    }
    
    void read_bytes(void* data, size_t size);
    std::string read_string();
    std::vector<uint8_t> read_vector();
    
    // Complex type reading
    TimePoint read_timepoint();
    Duration read_duration();
    glm::vec2 read_vec2();
    glm::vec3 read_vec3();
    glm::vec4 read_vec4();
    
    // Entity deserialization
    std::pair<entt::entity, TimelineEntity> read_entity();
    Clip read_clip();
    Track read_track();
    Marker read_marker();
    Transition read_transition();
    
    // Timeline deserialization
    bool deserialize_timeline(TimelineManager& timeline);
    
    // Validation
    bool validate_buffer() const;
    bool validate_data() const;
    
private:
    void decompress_buffer();
    bool read_and_validate_header();
    bool seek_to(size_t position);
};

// Auto-save and backup system
class TimelineAutoSave {
private:
    TimelineManager* timeline_;
    std::string project_path_;
    std::string auto_save_directory_;
    
    // Auto-save settings
    bool enabled_ = true;
    std::chrono::minutes auto_save_interval_{5};
    std::chrono::minutes backup_retention_{60 * 24}; // 24 hours
    size_t max_auto_saves_ = 10;
    
    // Auto-save state
    std::chrono::steady_clock::time_point last_auto_save_;
    std::thread auto_save_thread_;
    std::atomic<bool> auto_save_running_{false};
    
    // Backup management
    struct BackupInfo {
        std::string filename;
        std::chrono::system_clock::time_point timestamp;
        size_t file_size;
        bool is_auto_save;
    };
    
    std::vector<BackupInfo> backups_;
    
public:
    explicit TimelineAutoSave(TimelineManager* timeline);
    ~TimelineAutoSave();
    
    // Auto-save configuration
    void set_project_path(const std::string& path);
    void set_auto_save_directory(const std::string& directory);
    void set_enabled(bool enabled);
    void set_auto_save_interval(std::chrono::minutes interval);
    void set_backup_retention(std::chrono::minutes retention);
    void set_max_auto_saves(size_t count);
    
    // Auto-save operations
    void start_auto_save();
    void stop_auto_save();
    bool trigger_auto_save();
    
    // Backup management
    std::vector<BackupInfo> get_available_backups() const;
    bool restore_backup(const std::string& backup_filename);
    void cleanup_old_backups();
    
    // Recovery
    std::vector<std::string> find_crash_recovery_files() const;
    bool recover_from_crash(const std::string& recovery_file);
    
private:
    void auto_save_loop();
    std::string generate_auto_save_filename() const;
    std::string generate_backup_filename() const;
    
    void scan_existing_backups();
    void update_backup_list();
    
    bool save_timeline(const std::string& filename, SerializationFormat format);
    void log_auto_save(const std::string& message);
};

} // namespace Shotcut::Timeline
```

---

## 7.2 CLIP MANAGEMENT SYSTEM

### 7.2.1 Professional Clip Operations

**Clip Management Philosophy:**
- **Non-Destructive:** Preserve original media with effect stacks
- **Frame Accurate:** Microsecond precision for all operations
- **Performance Optimized:** Efficient operations on large timelines
- **Professional Features:** Ripple, slip, slide, trim, and JKL cuts

**src/timeline/clip_manager.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_set>

namespace Shotcut::Timeline {

// Edit operation types
enum class EditOperation : uint8_t {
    Insert,
    Remove,
    Move,
    TrimStart,
    TrimEnd,
    Split,
    RippleDelete,
    Slip,
    Slide,
    Replace,
    Duplicate
};

// Edit modes
enum class EditMode : uint8_t {
    Normal,         // Standard editing
    Ripple,         // Ripple edits affect subsequent clips
    Slip,           // Slip without changing duration
    Roll,           // Roll edit between two clips
    Overwrite,       // Overwrite existing content
    Insert          // Insert and push content
};

// Clip manager for professional editing operations
class ClipManager {
private:
    TimelineManager* timeline_;
    
    // Edit state
    EditMode current_mode_ = EditMode::Normal;
    std::vector<entt::entity> selected_clips_;
    std::unordered_set<entt::entity> locked_clips_;
    
    // Clipboard
    struct ClipboardEntry {
        Clip clip_data;
        std::string original_timeline_id;
        TimePoint original_position;
    };
    
    std::vector<ClipboardEntry> clipboard_;
    TimePoint clipboard_time_origin_ = Duration{0};
    
    // Edit history for undo/redo
    struct EditHistoryEntry {
        EditOperation operation;
        std::string description;
        TimePoint timestamp;
        std::vector<entt::entity> affected_entities;
        std::function<void()> undo_function;
        std::function<void()> redo_function;
    };
    
    std::vector<EditHistoryEntry> edit_history_;
    size_t history_index_ = 0;
    
    // Snapping
    bool snapping_enabled_ = true;
    Duration snap_threshold_ = std::chrono::milliseconds(100);
    std::vector<TimePoint> snap_points_;
    
    // Ripple editing
    bool ripple_mode_enabled_ = false;
    std::vector<entt::entity> ripple_affected_clips_;
    
public:
    explicit ClipManager(TimelineManager* timeline);
    ~ClipManager() = default;
    
    // Edit mode management
    void set_edit_mode(EditMode mode);
    EditMode get_edit_mode() const { return current_mode_; }
    
    // Basic clip operations
    entt::entity insert_clip(const std::string& media_path, const TimeRange& source_range,
                          TimePoint timeline_position, entt::entity target_track);
    
    bool remove_clip(entt::entity clip);
    bool remove_clips(const std::vector<entt::entity>& clips);
    
    bool move_clip(entt::entity clip, TimePoint new_position, entt::entity target_track);
    bool move_clips(const std::vector<entt::entity>& clips, 
                   TimePoint delta_time, int track_delta);
    
    // Trimming operations
    bool trim_clip_start(entt::entity clip, TimePoint new_start);
    bool trim_clip_end(entt::entity clip, TimePoint new_end);
    bool trim_clips_start(const std::vector<entt::entity>& clips, TimePoint new_start);
    bool trim_clips_end(const std::vector<entt::entity>& clips, TimePoint new_end);
    
    // Splitting operations
    std::vector<entt::entity> split_clip(entt::entity clip, TimePoint split_time);
    std::vector<entt::entity> split_clips_at_time(TimePoint time);
    std::vector<entt::entity> split_selected_clips_at_time(TimePoint time);
    
    // Ripple editing
    bool ripple_delete(entt::entity clip);
    bool ripple_delete_clips(const std::vector<entt::entity>& clips);
    bool ripple_insert(const std::string& media_path, const TimeRange& source_range,
                    TimePoint timeline_position, entt::entity target_track);
    
    // Slip and slide operations
    bool slip_clip(entt::entity clip, Duration offset);
    bool slide_clip(entt::entity clip, Duration offset);
    bool slip_clips(const std::vector<entt::entity>& clips, Duration offset);
    bool slide_clips(const std::vector<entt::entity>& clips, Duration offset);
    
    // Roll editing
    bool roll_edit(entt::entity left_clip, entt::entity right_clip, TimePoint edit_point);
    bool trim_between_clips(entt::entity left_clip, entt::entity right_clip, TimePoint edit_point);
    
    // Clipboard operations
    void copy_clips(const std::vector<entt::entity>& clips);
    void cut_clips(const std::vector<entt::entity>& clips);
    std::vector<entt::entity> paste_clips(TimePoint position, entt::entity target_track);
    void duplicate_clips(const std::vector<entt::entity>& clips);
    
    // Selection operations
    void select_clip(entt::entity clip, bool multi_select = false);
    void select_clips_in_range(const TimeRange& range, bool multi_select = false);
    void select_clips_on_track(entt::entity track, bool multi_select = false);
    void select_all_clips();
    void invert_clip_selection();
    void clear_clip_selection();
    
    const std::vector<entt::entity>& get_selected_clips() const { return selected_clips_; }
    bool is_clip_selected(entt::entity clip) const;
    
    // Locking operations
    void lock_clip(entt::entity clip);
    void unlock_clip(entt::entity clip);
    void lock_clips(const std::vector<entt::entity>& clips);
    void unlock_clips(const std::vector<entt::entity>& clips);
    bool is_clip_locked(entt::entity clip) const;
    
    // Snapping
    void enable_snapping(bool enabled);
    bool is_snapping_enabled() const { return snapping_enabled_; }
    void set_snap_threshold(Duration threshold);
    TimePoint snap_to_nearest(TimePoint time) const;
    void update_snap_points();
    
    // Clip properties
    void set_clip_speed(entt::entity clip, float speed);
    void set_clip_volume(entt::entity clip, float volume);
    void set_clip_opacity(entt::entity clip, float opacity);
    void set_clip_blend_mode(entt::entity clip, Clip::BlendMode mode);
    void set_clip_transform(entt::entity clip, const glm::vec2& position, 
                          const glm::vec2& scale, float rotation);
    
    // Keyframe operations
    void add_keyframe(entt::entity clip, TimePoint time, const std::string& parameter,
                    const std::variant<float, glm::vec2, glm::vec3, glm::vec4>& value,
                    const std::string& interpolation = "linear");
    void remove_keyframe(entt::entity clip, TimePoint time, const std::string& parameter);
    void update_keyframe(entt::entity clip, TimePoint time, const std::string& parameter,
                       const std::variant<float, glm::vec2, glm::vec3, glm::vec4>& value);
    
    std::vector<Clip::Keyframe> get_keyframes(entt::entity clip) const;
    std::vector<Clip::Keyframe> get_keyframes_in_range(entt::entity clip, const TimeRange& range) const;
    
    std::variant<float, glm::vec2, glm::vec3, glm::vec4> 
    interpolate_parameter(entt::entity clip, const std::string& parameter, TimePoint time) const;
    
    // Undo/Redo
    bool can_undo() const;
    bool can_redo() const;
    void undo();
    void redo();
    void clear_history();
    
    // Validation
    bool validate_clip_operations() const;
    std::vector<std::string> get_clip_errors() const;
    
    // Performance optimization
    void optimize_clip_storage();
    void rebuild_clip_indices();
    
private:
    // Helper functions
    TimePoint calculate_ripple_offset(const std::vector<entt::entity>& clips, bool is_delete) const;
    void update_ripple_affected_clips(const std::vector<entt::entity>& modified_clips);
    
    std::function<void()> create_clip_undo_function(entt::entity clip);
    std::function<void()> create_clip_redo_function(entt::entity clip);
    
    void add_edit_history(EditOperation operation, const std::string& description,
                        const std::vector<entt::entity>& affected_clips);
    
    bool validate_clip_operation(EditOperation operation, entt::entity clip) const;
    void log_clip_operation(EditOperation operation, entt::entity clip, const std::string& details);
    
    // Snapping helpers
    std::vector<TimePoint> get_clip_boundaries() const;
    std::vector<TimePoint> get_marker_positions() const;
    TimePoint find_nearest_snap_point(TimePoint time) const;
    
    // Keyframe helpers
    float interpolate_keyframes(const std::vector<Clip::Keyframe>& keyframes, 
                            TimePoint time) const;
    glm::vec2 interpolate_keyframes_vec2(const std::vector<Clip::Keyframe>& keyframes, 
                                      TimePoint time) const;
    glm::vec3 interpolate_keyframes_vec3(const std::vector<Clip::Keyframe>& keyframes, 
                                      TimePoint time) const;
    glm::vec4 interpolate_keyframes_vec4(const std::vector<Clip::Keyframe>& keyframes, 
                                      TimePoint time) const;
};

// Clip analyzer for media analysis and metadata extraction
class ClipAnalyzer {
private:
    struct MediaInfo {
        int width = 0;
        int height = 0;
        float frame_rate = 0.0f;
        Duration duration = Duration{0};
        PixelFormat pixel_format = PixelFormat::Unknown;
        std::vector<std::string> audio_streams;
        std::vector<std::string> subtitle_streams;
        std::unordered_map<std::string, std::string> metadata;
    };
    
    std::unordered_map<std::string, MediaInfo> media_cache_;
    
public:
    ClipAnalyzer() = default;
    ~ClipAnalyzer() = default;
    
    // Media analysis
    bool analyze_media(const std::string& media_path, MediaInfo& info);
    MediaInfo get_media_info(const std::string& media_path);
    void clear_media_cache();
    
    // Clip creation helpers
    entt::entity create_video_clip(TimelineManager* timeline, const std::string& media_path,
                                 TimePoint timeline_position, entt::entity target_track);
    entt::entity create_audio_clip(TimelineManager* timeline, const std::string& media_path,
                                 TimePoint timeline_position, entt::entity target_track);
    entt::entity create_image_clip(TimelineManager* timeline, const std::string& image_path,
                                  TimePoint timeline_position, Duration duration, entt::entity target_track);
    entt::entity create_title_clip(TimelineManager* timeline, const std::string& title,
                                 const std::string& font, float font_size, const glm::vec4& color,
                                 TimePoint timeline_position, Duration duration, entt::entity target_track);
    
    // Smart clip creation
    entt::entity create_clip_from_media(TimelineManager* timeline, const std::string& media_path,
                                     TimePoint timeline_position, entt::entity target_track);
    std::vector<entt::entity> create_multi_clip_from_media(TimelineManager* timeline, 
                                                         const std::string& media_path,
                                                         TimePoint timeline_position, 
                                                         entt::entity target_track);
    
private:
    bool extract_media_metadata(const std::string& media_path, MediaInfo& info);
    bool generate_thumbnail(const std::string& media_path, const std::string& thumbnail_path);
    bool generate_waveform(const std::string& media_path, const std::string& waveform_path);
    
    void cache_media_info(const std::string& media_path, const MediaInfo& info);
    bool is_cached(const std::string& media_path) const;
};

} // namespace Shotcut::Timeline
```

---

## 7.3 TRACK SYSTEM IMPLEMENTATION

### 7.3.1 Multi-Track Architecture

**Track System Philosophy:**
- **Flexible Track Types:** Video, audio, subtitle, effect, and marker tracks
- **Track Grouping:** Parent-child relationships for complex arrangements
- **Track Compositing**: Sophisticated blend modes and compositing
- **Performance Optimization**: Efficient track rendering and culling

**src/timeline/track_system.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Shotcut::Timeline {

// Track composition modes
enum class TrackCompositionMode : uint8_t {
    Normal,        // Standard overlay
    Add,           // Additive blending
    Multiply,       // Multiply blending
    Screen,        // Screen blending
    Overlay,       // Overlay blending
    Darken,        // Darken only
    Lighten,       // Lighten only
    Difference,     // Difference blending
    Exclusion       // Exclusion blending
};

// Track height modes
enum class TrackHeightMode : uint8_t {
    Fixed,          // Fixed height in pixels
    Auto,           // Auto-adjust based on content
    Proportional    // Proportional to other tracks
};

// Track system manager
class TrackSystem {
private:
    TimelineManager* timeline_;
    
    // Track hierarchy
    struct TrackNode {
        entt::entity track;
        entt::entity parent;     // entt::null for root
        std::vector<entt::entity> children;
        int depth = 0;
        bool is_collapsed = false;
    };
    
    std::unordered_map<entt::entity, TrackNode> track_hierarchy_;
    std::vector<entt::entity> root_tracks_;
    
    // Track rendering
    struct TrackRenderData {
        VkImage render_target;
        VkImageView render_view;
        VkSampler sampler;
        VkDescriptorSet descriptor_set;
        bool needs_update = true;
    };
    
    std::unordered_map<entt::entity, TrackRenderData> track_render_data_;
    VkDevice vulkan_device_;
    
    // Track composition
    std::vector<entt::entity> visible_tracks_;
    std::unordered_map<entt::entity, TrackCompositionMode> track_composition_;
    std::unordered_map<entt::entity, float> track_opacity_;
    
    // Track height management
    TrackHeightMode height_mode_ = TrackHeightMode::Auto;
    float default_track_height_ = 50.0f;
    float min_track_height_ = 20.0f;
    float max_track_height_ = 200.0f;
    
public:
    explicit TrackSystem(TimelineManager* timeline, VkDevice vulkan_device);
    ~TrackSystem();
    
    // Track creation and management
    entt::entity create_track(const std::string& name, Track::Type type, 
                           entt::entity parent = entt::null);
    void remove_track(entt::entity track);
    void move_track(entt::entity track, entt::entity new_parent, int index = -1);
    
    // Track hierarchy
    void set_track_parent(entt::entity track, entt::entity parent);
    entt::entity get_track_parent(entt::entity track) const;
    const std::vector<entt::entity>& get_track_children(entt::entity track) const;
    
    void collapse_track(entt::entity track);
    void expand_track(entt::entity track);
    bool is_track_collapsed(entt::entity track) const;
    
    int get_track_depth(entt::entity track) const;
    std::vector<entt::entity> get_tracks_at_depth(int depth) const;
    
    // Track ordering
    void move_track_up(entt::entity track);
    void move_track_down(entt::entity track);
    void move_track_to_index(entt::entity track, int index);
    
    int get_track_index(entt::entity track) const;
    entt::entity get_track_at_index(int index) const;
    
    // Track properties
    void set_track_name(entt::entity track, const std::string& name);
    void set_track_type(entt::entity track, Track::Type type);
    void set_track_height(entt::entity track, float height);
    void set_track_locked(entt::entity track, bool locked);
    void set_track_muted(entt::entity track, bool muted);
    void set_track_soloed(entt::entity track, bool soloed);
    
    // Track composition
    void set_track_composition_mode(entt::entity track, TrackCompositionMode mode);
    void set_track_opacity(entt::entity track, float opacity);
    void set_track_visible(entt::entity track, bool visible);
    
    TrackCompositionMode get_track_composition_mode(entt::entity track) const;
    float get_track_opacity(entt::entity track) const;
    bool is_track_visible(entt::entity track) const;
    
    // Track height management
    void set_height_mode(TrackHeightMode mode);
    void set_default_track_height(float height);
    void set_track_height_limits(float min_height, float max_height);
    
    void auto_adjust_track_heights();
    void recalculate_track_layout();
    
    // Track rendering
    bool initialize_track_rendering(entt::entity track);
    void cleanup_track_rendering(entt::entity track);
    void update_track_render_target(entt::entity track);
    
    VkImage get_track_render_target(entt::entity track) const;
    VkDescriptorSet get_track_descriptor_set(entt::entity track) const;
    
    void render_track(entt::entity track, VkCommandBuffer command_buffer, 
                    TimePoint start_time, TimePoint end_time);
    void render_tracks(VkCommandBuffer command_buffer, 
                    TimePoint start_time, TimePoint end_time);
    
    // Track queries
    std::vector<entt::entity> get_tracks_of_type(Track::Type type) const;
    std::vector<entt::entity> get_visible_tracks() const;
    std::vector<entt::entity> get_tracks_in_range(TimePoint start_time, TimePoint end_time) const;
    
    entt::entity get_track_at_point(float y) const;
    std::vector<entt::entity> get_tracks_in_rect(float x, float y, float width, float height) const;
    
    // Track validation
    bool validate_track_hierarchy() const;
    std::vector<std::string> get_track_errors() const;
    
private:
    void update_track_hierarchy(entt::entity track);
    void update_track_depth(entt::entity track, int depth);
    
    bool create_track_render_resources(entt::entity track);
    void destroy_track_render_resources(entt::entity track);
    
    void calculate_visible_tracks();
    void update_track_render_order();
    
    float calculate_optimal_track_height(entt::entity track) const;
    void redistribute_track_heights();
};

// Specialized track managers

class VideoTrackManager {
private:
    TrackSystem* track_system_;
    TimelineManager* timeline_;
    
    // Video track rendering
    struct VideoRenderContext {
        VkPipeline pipeline;
        VkPipelineLayout layout;
        VkDescriptorSetLayout descriptor_layout;
        VkSampler sampler;
        
        // Shaders
        VkShaderModule vertex_shader;
        VkShaderModule fragment_shader;
        
        // Render targets
        std::array<VkImage, MAX_FRAMES_IN_FLIGHT> render_images;
        std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> render_views;
        std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> framebuffers;
    };
    
    VideoRenderContext render_context_;
    
    // Video track features
    bool thumbnails_enabled_ = true;
    bool waveforms_enabled_ = false;
    float thumbnail_quality_ = 0.5f;
    float thumbnail_resolution_ = 64.0f;
    
public:
    explicit VideoTrackManager(TrackSystem* track_system, TimelineManager* timeline);
    ~VideoTrackManager();
    
    // Video track operations
    bool initialize_rendering(VkDevice device, VkRenderPass render_pass);
    void cleanup_rendering();
    
    void render_video_track(entt::entity track, VkCommandBuffer command_buffer,
                         TimePoint start_time, TimePoint end_time);
    
    // Video track features
    void enable_thumbnails(bool enabled);
    void enable_waveforms(bool enabled);
    void set_thumbnail_quality(float quality);
    void set_thumbnail_resolution(float resolution);
    
    // Video track properties
    void set_track_background_color(entt::entity track, const glm::vec4& color);
    void set_track_opacity(entt::entity track, float opacity);
    void set_track_blend_mode(entt::entity track, TrackCompositionMode mode);
    
    // Video clip rendering
    void render_video_clip(entt::entity clip, VkCommandBuffer command_buffer,
                         TimePoint start_time, TimePoint end_time);
    
private:
    bool create_render_pipeline(VkDevice device, VkRenderPass render_pass);
    void destroy_render_pipeline();
    
    VkShaderModule create_vertex_shader();
    VkShaderModule create_fragment_shader();
};

class AudioTrackManager {
private:
    TrackSystem* track_system_;
    TimelineManager* timeline_;
    
    // Audio track rendering
    struct AudioRenderContext {
        VkImage waveform_image;
        VkImageView waveform_view;
        VkDeviceMemory waveform_memory;
        
        // Waveform data
        std::vector<float> waveform_data;
        uint32_t waveform_width = 0;
        uint32_t waveform_height = 128;
        
        // Audio levels
        std::vector<float> left_levels;
        std::vector<float> right_levels;
        uint32_t levels_resolution = 1000;
    };
    
    std::unordered_map<entt::entity, AudioRenderContext> audio_contexts_;
    
    // Audio track features
    bool waveforms_enabled_ = true;
    bool levels_enabled_ = true;
    float waveform_scale_ = 1.0f;
    AudioProperties::Metering metering_mode_ = AudioProperties::Metering::Peak;
    
public:
    explicit AudioTrackManager(TrackSystem* track_system, TimelineManager* timeline);
    ~AudioTrackManager();
    
    // Audio track operations
    void render_audio_track(entt::entity track, VkCommandBuffer command_buffer,
                         TimePoint start_time, TimePoint end_time);
    
    // Audio track features
    void enable_waveforms(bool enabled);
    void enable_levels(bool enabled);
    void set_waveform_scale(float scale);
    void set_metering_mode(AudioProperties::Metering mode);
    
    // Audio track properties
    void set_track_pan(entt::entity track, float pan);
    void set_track_gain(entt::entity track, float gain);
    void set_track_volume(entt::entity track, float volume);
    
    // Audio waveform generation
    void generate_waveform(entt::entity track);
    void update_audio_levels(entt::entity track);
    
private:
    bool create_audio_context(entt::entity track);
    void destroy_audio_context(entt::entity track);
    
    void generate_waveform_data(const std::string& audio_path, std::vector<float>& data);
    void calculate_audio_levels(const std::string& audio_path, 
                           std::vector<float>& left_levels, 
                           std::vector<float>& right_levels);
};

class SubtitleTrackManager {
private:
    TrackSystem* track_system_;
    TimelineManager* timeline_;
    
    // Subtitle rendering
    struct SubtitleRenderContext {
        VkImage text_image;
        VkImageView text_view;
        VkSampler sampler;
        
        // Font rendering
        void* font_context;
        void* font_face;
        
        // Subtitle cache
        std::unordered_map<std::string, VkImage> text_cache;
    };
    
    std::unordered_map<entt::entity, SubtitleRenderContext> subtitle_contexts_;
    
    // Subtitle features
    bool subtitles_enabled_ = true;
    float subtitle_scale_ = 1.0f;
    glm::vec4 subtitle_color_ = glm::vec4(1.0f);
    
public:
    explicit SubtitleTrackManager(TrackSystem* track_system, TimelineManager* timeline);
    ~SubtitleTrackManager();
    
    // Subtitle track operations
    void render_subtitle_track(entt::entity track, VkCommandBuffer command_buffer,
                             TimePoint start_time, TimePoint end_time);
    
    // Subtitle features
    void enable_subtitles(bool enabled);
    void set_subtitle_scale(float scale);
    void set_subtitle_color(const glm::vec4& color);
    
    // Subtitle track properties
    void set_track_font(entt::entity track, const std::string& font_family, float font_size);
    void set_track_text_color(entt::entity track, const glm::vec4& color);
    void set_track_background_color(entt::entity track, const glm::vec4& color);
    void set_track_outline(entt::entity track, float width, const glm::vec4& color);
    
    // Subtitle rendering
    void render_subtitle_text(entt::entity track, const std::string& text,
                             VkCommandBuffer command_buffer);
    
private:
    bool create_subtitle_context(entt::entity track);
    void destroy_subtitle_context(entt::entity track);
    
    void render_text_to_image(const std::string& text, SubtitleRenderContext& context);
    void update_subtitle_cache(const std::string& text, SubtitleRenderContext& context);
};

} // namespace Shotcut::Timeline
```

---

## 7.4 BASIC EDITING OPERATIONS

### 7.4.1 Professional Editing Tools

**Editing Operations Philosophy:**
- **Professional Precision:** Frame-accurate editing with microsecond precision
- **Non-Destructive:** Preserve original media with effect stacks
- **Performance Optimized:** Efficient operations on complex timelines
- **Industry Standard:** Familiar editing paradigms from professional NLEs

**src/timeline/editing_operations.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include "clip_manager.h"
#include <memory>
#include <vector>
#include <functional>

namespace Shotcut::Timeline {

// Editing tool types
enum class EditingTool : uint8_t {
    Select,         // Selection tool
    Razor,          // Cut/split tool
    RippleEdit,     // Ripple edit tool
    Slip,           // Slip tool
    Slide,          // Slide tool
    Trim,           // Trim tool
    Pen,            // Pen tool for keyframes
    Hand,           // Pan tool
    Zoom            // Zoom tool
};

// Edit constraints
struct EditConstraints {
    bool preserve_duration = false;
    bool preserve_position = false;
    bool snap_to_frames = true;
    bool snap_to_markers = true;
    bool snap_to_clips = true;
    Duration min_clip_duration = std::chrono::milliseconds(1);
    Duration max_clip_duration = Duration{0}; // 0 = unlimited
    bool prevent_overlaps = false;
    bool maintain_sync = true;
};

// Professional editing operations manager
class EditingOperations {
private:
    TimelineManager* timeline_;
    ClipManager* clip_manager_;
    
    // Current editing state
    EditingTool current_tool_ = EditingTool::Select;
    EditConstraints constraints_;
    
    // Edit operations
    std::vector<entt::entity> selected_clips_;
    entt::entity primary_clip_ = entt::null;
    TimePoint edit_point_ = Duration{0};
    entt::entity target_track_ = entt::null;
    
    // Preview state
    bool preview_enabled_ = true;
    TimePoint preview_start_ = Duration{0};
    TimePoint preview_end_ = Duration{0};
    
    // Edit modes
    bool insert_mode_ = false;
    bool overwrite_mode_ = false;
    bool snap_enabled_ = true;
    Duration snap_threshold_ = std::chrono::milliseconds(100);
    
public:
    explicit EditingOperations(TimelineManager* timeline, ClipManager* clip_manager);
    ~EditingOperations() = default;
    
    // Tool management
    void set_tool(EditingTool tool);
    EditingTool get_tool() const { return current_tool_; }
    
    // Edit constraints
    void set_constraints(const EditConstraints& constraints);
    const EditConstraints& get_constraints() const { return constraints_; }
    
    // Selection operations
    void select_clips_at_point(float x, float y, bool multi_select = false);
    void select_clips_in_rect(float x, float y, float width, float height, bool multi_select = false);
    void select_all_clips();
    void clear_selection();
    void invert_selection();
    
    // Basic editing operations
    void cut_clips(const std::vector<entt::entity>& clips);
    void copy_clips(const std::vector<entt::entity>& clips);
    void paste_clips_at_time(TimePoint time);
    void delete_clips(const std::vector<entt::entity>& clips);
    
    // Trim operations
    void trim_clip_start_at_point(entt::entity clip, TimePoint trim_point);
    void trim_clip_end_at_point(entt::entity clip, TimePoint trim_point);
    void trim_multiple_clips_start(const std::vector<entt::entity>& clips, TimePoint trim_point);
    void trim_multiple_clips_end(const std::vector<entt::entity>& clips, TimePoint trim_point);
    
    // Razor/split operations
    void split_clip_at_time(entt::entity clip, TimePoint split_time);
    void split_all_clips_at_time(TimePoint split_time);
    void split_selected_clips_at_time(TimePoint split_time);
    
    // Ripple editing
    void ripple_delete_clip(entt::entity clip);
    void ripple_delete_clips(const std::vector<entt::entity>& clips);
    void ripple_insert_clip(const std::string& media_path, TimePoint position, entt::entity target_track);
    void ripple_trim_clip_start(entt::entity clip, TimePoint new_start);
    void ripple_trim_clip_end(entt::entity clip, TimePoint new_end);
    
    // Slip and slide operations
    void slip_clip(entt::entity clip, Duration offset);
    void slide_clip(entt::entity clip, Duration offset);
    void slip_selected_clips(Duration offset);
    void slide_selected_clips(Duration offset);
    
    // Roll editing
    void roll_edit_between_clips(entt::entity left_clip, entt::entity right_clip, TimePoint edit_point);
    void trim_adjacent_clips(entt::entity clip, TimePoint edit_point);
    
    // Drag and drop
    void begin_clip_drag(const std::vector<entt::entity>& clips);
    void update_clip_drag(const glm::vec2& delta);
    void finish_clip_drag();
    
    void begin_clip_trim(entt::entity clip, bool trim_start);
    void update_clip_trim(const glm::vec2& position);
    void finish_clip_trim();
    
    // Multicam editing
    void enable_multicam_mode(bool enabled);
    void set_multicam_angles(int angle_count);
    void switch_multicam_angle(int angle);
    
    // JKL editing (professional)
    void jkl_cut_at_playhead();
    void jkl_trim_before_playhead();
    void jkl_trim_after_playhead();
    void jkl_extend_selection_to_playhead();
    
    // Markers
    void add_marker_at_time(TimePoint time, const std::string& name = "");
    void remove_marker_at_time(TimePoint time);
    void edit_marker_at_time(TimePoint time, const std::string& new_name);
    
    // Preview operations
    void enable_preview(bool enabled);
    void set_preview_range(TimePoint start, TimePoint end);
    void clear_preview_range();
    
    void preview_edit_operation();
    void stop_preview();
    
    // Snap operations
    void enable_snap(bool enabled);
    void set_snap_threshold(Duration threshold);
    
    TimePoint snap_time(TimePoint time) const;
    glm::vec2 snap_position(const glm::vec2& position) const;
    
    // Validation
    bool validate_edit_operation() const;
    std::vector<std::string> get_edit_errors() const;
    
private:
    // Helper functions
    TimePoint calculate_snap_point(TimePoint time) const;
    std::vector<entt::entity> get_clips_affected_by_edit(const std::vector<entt::entity>& clips) const;
    
    bool can_perform_edit_on_clip(entt::entity clip) const;
    bool can_move_clip_to_position(entt::entity clip, TimePoint position, entt::entity track) const;
    
    void update_edit_preview();
    void clear_edit_preview();
    
    void log_edit_operation(const std::string& operation, const std::vector<entt::entity>& clips);
    
    // Constraint checking
    bool check_duration_constraint(entt::entity clip, Duration new_duration) const;
    bool check_position_constraint(entt::entity clip, TimePoint new_position) const;
    bool check_overlap_constraint(const std::vector<entt::entity>& clips, 
                             TimePoint new_position, entt::entity track) const;
    bool check_sync_constraint(const std::vector<entt::entity>& clips) const;
};

// Advanced editing operations
class AdvancedEditingOperations {
private:
    TimelineManager* timeline_;
    ClipManager* clip_manager_;
    
    // Compositing
    bool compositing_enabled_ = true;
    TrackCompositionMode default_composition_ = TrackCompositionMode::Normal;
    
    // Automation
    bool automation_enabled_ = true;
    std::unordered_map<std::string, std::vector<Clip::Keyframe>> automation_data_;
    
public:
    explicit AdvancedEditingOperations(TimelineManager* timeline, ClipManager* clip_manager);
    ~AdvancedEditingOperations() = default;
    
    // Compositing operations
    void enable_compositing(bool enabled);
    void set_default_composition_mode(TrackCompositionMode mode);
    
    void set_track_composition(entt::entity track, TrackCompositionMode mode);
    void set_track_opacity(entt::entity track, float opacity);
    void composite_tracks(const std::vector<entt::entity>& tracks);
    
    // Automation operations
    void enable_automation(bool enabled);
    void add_automation_keyframe(entt::entity clip, TimePoint time, const std::string& parameter,
                               const std::variant<float, glm::vec2, glm::vec3, glm::vec4>& value);
    void remove_automation_keyframe(entt::entity clip, TimePoint time, const std::string& parameter);
    
    std::vector<Clip::Keyframe> get_automation_keyframes(entt::entity clip, const std::string& parameter) const;
    float interpolate_automation_value(entt::entity clip, const std::string& parameter, TimePoint time) const;
    
    // Speed ramping
    void create_speed_ramp(entt::entity clip, TimePoint start_time, float start_speed,
                          TimePoint end_time, float end_speed);
    void reverse_clip_speed(entt::entity clip);
    
    // Frame interpolation
    void enable_frame_interpolation(entt::entity clip, bool enabled);
    void set_interpolation_quality(entt::entity clip, float quality);
    
    // Freeze frames
    void freeze_frame(entt::entity clip, TimePoint freeze_time, Duration freeze_duration);
    
    // Time remapping
    void create_time_remap(entt::entity clip, const std::vector<std::pair<TimePoint, TimePoint>>& remap_points);
    void smooth_time_remap(entt::entity clip);
    
    // Nesting
    entt::entity create_nested_sequence(const std::vector<entt::entity>& clips,
                                    const std::string& sequence_name);
    void flatten_nested_sequence(entt::entity nested_clip);
    
    // Multi-angle editing
    void create_multi_angle_clip(const std::vector<std::string>& media_paths,
                              TimePoint position, entt::entity target_track);
    void switch_angle(entt::entity multi_angle_clip, int angle);
    
    // Color grading automation
    void add_color_keyframe(entt::entity clip, TimePoint time, const glm::vec4& color_correction);
    void remove_color_keyframe(entt::entity clip, TimePoint time);
    
    void create_color_ramp(entt::entity clip, TimePoint start_time, const glm::vec4& start_color,
                         TimePoint end_time, const glm::vec4& end_color);
    
private:
    void update_automation_data(entt::entity clip, const std::string& parameter);
    void render_composited_track(entt::entity track);
    
    std::vector<float> generate_interpolation_curve(const std::vector<std::pair<TimePoint, float>>& points,
                                               const std::string& interpolation_type) const;
};

// Editing operation factory for creating operations with undo/redo support
class EditOperationFactory {
public:
    struct EditOperation {
        std::string description;
        std::function<void()> execute;
        std::function<void()> undo;
        std::function<void()> redo;
        std::vector<entt::entity> affected_entities;
    };
    
    static EditOperation create_move_clip(TimelineManager* timeline, entt::entity clip, 
                                     TimePoint old_position, TimePoint new_position,
                                     entt::entity old_track, entt::entity new_track);
    
    static EditOperation create_trim_clip(TimelineManager* timeline, entt::entity clip,
                                     TimeRange old_range, TimeRange new_range);
    
    static EditOperation create_split_clip(TimelineManager* timeline, entt::entity clip,
                                      TimePoint split_time);
    
    static EditOperation create_delete_clips(TimelineManager* timeline, 
                                        const std::vector<entt::entity>& clips);
    
    static EditOperation create_ripple_edit(TimelineManager* timeline,
                                       const std::vector<entt::entity>& clips,
                                       Duration offset);
    
private:
    static std::function<void()> create_clip_restore_function(TimelineManager* timeline,
                                                        entt::entity clip,
                                                        const Clip& original_state);
};

} // namespace Shotcut::Timeline
```

---

## 7.5 DRAG-AND-DROP FUNCTIONALITY

### 7.5.1 Professional Drag and Drop System

**Drag and Drop Philosophy:**
- **Multi-Format Support:** Media files, clips, effects, and transitions
- **Visual Feedback:** Real-time preview during drag operations
- **Smart Positioning:** Intelligent snap and positioning
- **Performance Optimized:** Smooth drag operations with large timelines

**src/timeline/drag_drop.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Shotcut::Timeline {

// Drag and drop data types
enum class DragDataType : uint8_t {
    MediaFile,      // Media file from file system
    TimelineClip,    // Clip from timeline
    Effect,          // Effect from effects panel
    Transition,     // Transition from transitions panel
    Marker,         // Marker from markers panel
    Text,           // Text from text tool
    URL,            // URL from browser
    Custom          // Custom data type
};

// Drag and drop data
struct DragData {
    DragDataType type;
    std::vector<uint8_t> data;
    std::string mime_type;
    std::string description;
    
    // Position info
    glm::vec2 start_position;
    glm::vec2 current_position;
    glm::vec2 hot_spot = glm::vec2(0.5f); // Hot spot relative to item size
    
    // Preview
    bool preview_enabled = true;
    std::string preview_image_path;
    glm::vec2 preview_size = glm::vec2(128.0f, 128.0f);
    
    // Constraints
    bool can_drop_on_timeline = true;
    bool can_drop_on_track = true;
    bool can_drop_on_clip = true;
    
    template<typename T>
    void set_data(const T& value) {
        data.resize(sizeof(T));
        memcpy(data.data(), &value, sizeof(T));
    }
    
    template<typename T>
    T get_data() const {
        if (data.size() >= sizeof(T)) {
            T value;
            memcpy(&value, data.data(), sizeof(T));
            return value;
        }
        return T{};
    }
};

// Drop target interface
class IDropTarget {
public:
    virtual ~IDropTarget() = default;
    
    virtual bool can_accept_drop(const DragData& drag_data, const glm::vec2& position) = 0;
    virtual void on_drag_enter(const DragData& drag_data, const glm::vec2& position) = 0;
    virtual void on_drag_over(const DragData& drag_data, const glm::vec2& position) = 0;
    virtual void on_drag_exit(const DragData& drag_data, const glm::vec2& position) = 0;
    virtual bool on_drop(const DragData& drag_data, const glm::vec2& position) = 0;
    virtual void on_drag_finished() = 0;
    
    virtual glm::vec4 get_drop_highlight_color() const { return glm::vec4(0.2f, 0.4f, 0.8f, 0.5f); }
    virtual glm::vec2 get_drop_position(const glm::vec2& mouse_pos) const = 0;
};

// Drag and drop manager
class DragDropManager {
private:
    static std::unique_ptr<DragDropManager> instance_;
    
    // Current drag state
    bool drag_active_ = false;
    DragData current_drag_data_;
    IDropTarget* current_target_ = nullptr;
    glm::vec2 last_mouse_position_;
    
    // Drop targets
    std::vector<std::weak_ptr<IDropTarget>> drop_targets_;
    std::unordered_map<DragDataType, std::vector<std::weak_ptr<IDropTarget>>> target_map_;
    
    // Visual feedback
    bool preview_enabled_ = true;
    VkImage preview_image_;
    VkImageView preview_view_;
    VkSampler preview_sampler_;
    glm::vec2 preview_position_;
    
    // Constraints
    std::vector<glm::vec4> no_drop_zones_;
    std::vector<entt::entity> locked_tracks_;
    
    // Performance
    std::chrono::steady_clock::time_point last_update_;
    constexpr static auto UPDATE_INTERVAL = std::chrono::milliseconds(16); // 60 FPS
    
public:
    static DragDropManager& get() {
        if (!instance_) {
            instance_ = std::make_unique<DragDropManager>();
        }
        return *instance_;
    }
    
    // Drag operations
    bool start_drag(const DragData& drag_data, const glm::vec2& start_position);
    void update_drag(const glm::vec2& current_position);
    void cancel_drag();
    void finish_drag();
    
    bool is_drag_active() const { return drag_active_; }
    const DragData& get_current_drag_data() const { return current_drag_data_; }
    
    // Drop target management
    void register_drop_target(std::shared_ptr<IDropTarget> target, DragDataType type);
    void unregister_drop_target(std::shared_ptr<IDropTarget> target);
    void unregister_all_targets();
    
    // Visual feedback
    void enable_preview(bool enabled);
    void set_no_drop_zones(const std::vector<glm::vec4>& zones);
    void set_locked_tracks(const std::vector<entt::entity>& tracks);
    
    // Rendering
    void render_drag_preview(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_drop_indicators(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    
    // Utility functions
    bool is_point_in_no_drop_zone(const glm::vec2& point) const;
    entt::entity get_track_at_position(const glm::vec2& position) const;
    
private:
    void update_current_target(const glm::vec2& position);
    void clear_current_target();
    
    bool create_preview_resources();
    void destroy_preview_resources();
    void update_preview_position(const glm::vec2& position);
    
    void render_preview_image(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_drop_zone_highlight(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
};

// Timeline drop target implementation
class TimelineDropTarget : public IDropTarget {
private:
    TimelineManager* timeline_;
    TrackSystem* track_system_;
    
    // Drop validation
    std::unordered_map<DragDataType, std::function<bool(const DragData&, const glm::vec2&)>> validators_;
    
    // Drop handlers
    std::unordered_map<DragDataType, std::function<bool(const DragData&, const glm::vec2&)>> handlers_;
    
public:
    explicit TimelineDropTarget(TimelineManager* timeline, TrackSystem* track_system);
    ~TimelineDropTarget() override = default;
    
    // IDropTarget interface
    bool can_accept_drop(const DragData& drag_data, const glm::vec2& position) override;
    void on_drag_enter(const DragData& drag_data, const glm::vec2& position) override;
    void on_drag_over(const DragData& drag_data, const glm::vec2& position) override;
    void on_drag_exit(const DragData& drag_data, const glm::vec2& position) override;
    bool on_drop(const DragData& drag_data, const glm::vec2& position) override;
    void on_drag_finished() override;
    
    glm::vec4 get_drop_highlight_color() const override;
    glm::vec2 get_drop_position(const glm::vec2& mouse_pos) const override;
    
private:
    // Validation functions
    bool can_accept_media_file(const DragData& drag_data, const glm::vec2& position);
    bool can_accept_timeline_clip(const DragData& drag_data, const glm::vec2& position);
    bool can_accept_effect(const DragData& drag_data, const glm::vec2& position);
    bool can_accept_transition(const DragData& drag_data, const glm::vec2& position);
    
    // Drop handlers
    bool handle_media_file_drop(const DragData& drag_data, const glm::vec2& position);
    bool handle_timeline_clip_drop(const DragData& drag_data, const glm::vec2& position);
    bool handle_effect_drop(const DragData& drag_data, const glm::vec2& position);
    bool handle_transition_drop(const DragData& drag_data, const glm::vec2& position);
    
    // Helper functions
    entt::entity get_target_track(const glm::vec2& position) const;
    TimePoint get_drop_time(const glm::vec2& position) const;
    
    void show_drop_feedback(const glm::vec2& position, bool can_drop);
    void clear_drop_feedback();
};

// Media file drag data specialization
struct MediaFileDragData {
    std::vector<std::string> file_paths;
    std::vector<std::string> mime_types;
    
    // File analysis results
    std::vector<ClipAnalyzer::MediaInfo> media_info;
    
    // Import options
    bool create_sequence = false;
    bool import_subtitles = true;
    bool import_audio_tracks = true;
    
    // Position options
    bool auto_position = true;
    TimePoint target_time = Duration{0};
    entt::entity target_track = entt::null;
};

// Effect drag data specialization
struct EffectDragData {
    std::string effect_id;
    std::string effect_name;
    std::string effect_category;
    
    // Effect parameters
    std::unordered_map<std::string, std::variant<float, std::string, glm::vec4>> default_parameters;
    
    // Application options
    bool apply_to_selected_clips = false;
    bool create_new_clip = false;
    float default_duration_seconds = 5.0f;
};

} // namespace Shotcut::Timeline
```

---

## 7.6 SELECTION AND MANIPULATION TOOLS

### 7.6.1 Advanced Selection System

**Selection System Philosophy:**
- **Flexible Selection:** Multiple selection modes and tools
- **Visual Feedback:** Clear selection indicators and highlights
- **Performance Optimized:** Efficient selection operations on large timelines
- **Keyboard Shortcuts:** Professional keyboard navigation and selection

**src/timeline/selection_system.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include <functional>

namespace Shotcut::Timeline {

// Selection modes
enum class SelectionMode : uint8_t {
    Normal,         // Click to select, Ctrl+click to multi-select
    Additive,       // Always add to selection
    Subtractive,    // Always remove from selection
    Toggle,         // Toggle selection state
    Lasso,          // Lasso selection
    Range           // Range selection
};

// Selection types
enum class SelectionType : uint8_t {
    Clip,           // Select clips
    Track,          // Select tracks
    Keyframe,       // Select keyframes
    Marker,         // Select markers
    Transition      // Select transitions
    All             // Select everything
};

// Selection region
struct SelectionRegion {
    TimePoint start_time;
    TimePoint end_time;
    int start_track = -1;
    int end_track = -1;
    bool is_valid = false;
    
    bool contains_time(TimePoint time) const {
        return time >= start_time && time <= end_time;
    }
    
    bool contains_track(int track_index) const {
        return track_index >= start_track && track_index <= end_track;
    }
    
    bool contains_point(TimePoint time, int track_index) const {
        return contains_time(time) && contains_track(track_index);
    }
};

// Selection manager
class SelectionManager {
private:
    TimelineManager* timeline_;
    
    // Selection state
    SelectionMode current_mode_ = SelectionMode::Normal;
    std::unordered_set<entt::entity> selected_entities_;
    std::unordered_set<entt::entity> selected_tracks_;
    std::unordered_set<entt::entity> selected_markers_;
    std::unordered_set<Clip::Keyframe> selected_keyframes_;
    
    // Selection history
    struct SelectionHistory {
        std::unordered_set<entt::entity> selected_entities;
        std::unordered_set<entt::entity> selected_tracks;
        SelectionMode mode;
        TimePoint timestamp;
    };
    
    std::vector<SelectionHistory> selection_history_;
    size_t history_index_ = 0;
    size_t max_history_size_ = 50;
    
    // Selection region
    SelectionRegion current_region_;
    bool region_selection_active_ = false;
    glm::vec2 region_start_;
    glm::vec2 region_end_;
    
    // Keyboard selection
    bool keyboard_selection_active_ = false;
    entt::entity keyboard_focus_entity_ = entt::null;
    Direction last_keyboard_direction_ = Direction::None;
    
    // Selection constraints
    bool lock_selection_ = false;
    SelectionType allowed_selection_types_ = SelectionType::All;
    
    // Callbacks
    std::function<void(const std::vector<entt::entity>&)> on_selection_changed_;
    std::function<void(SelectionType, int)> on_selection_count_changed_;
    
public:
    explicit SelectionManager(TimelineManager* timeline);
    ~SelectionManager() = default;
    
    // Selection mode management
    void set_selection_mode(SelectionMode mode);
    SelectionMode get_selection_mode() const { return current_mode_; }
    
    // Basic selection operations
    void select_entity(entt::entity entity, bool multi_select = false);
    void select_entities(const std::vector<entt::entity>& entities);
    void select_track(entt::entity track, bool multi_select = false);
    void select_tracks(const std::vector<entt::entity>& tracks);
    
    void deselect_entity(entt::entity entity);
    void deselect_entities(const std::vector<entt::entity>& entities);
    void deselect_all();
    
    void toggle_entity_selection(entt::entity entity);
    void toggle_track_selection(entt::entity track);
    
    // Selection queries
    bool is_entity_selected(entt::entity entity) const;
    bool is_track_selected(entt::entity track) const;
    bool is_marker_selected(entt::entity marker) const;
    bool is_keyframe_selected(const Clip::Keyframe& keyframe) const;
    
    const std::unordered_set<entt::entity>& get_selected_entities() const { return selected_entities_; }
    const std::unordered_set<entt::entity>& get_selected_tracks() const { return selected_tracks_; }
    std::vector<entt::entity> get_selected_entities_vector() const;
    std::vector<entt::entity> get_selected_tracks_vector() const;
    
    size_t get_selected_count(SelectionType type = SelectionType::All) const;
    
    // Region selection
    void begin_region_selection(const glm::vec2& start_point);
    void update_region_selection(const glm::vec2& current_point);
    void end_region_selection();
    void cancel_region_selection();
    
    bool is_region_selection_active() const { return region_selection_active_; }
    const SelectionRegion& get_selection_region() const { return current_region_; }
    
    // Lasso selection
    void begin_lasso_selection(const glm::vec2& start_point);
    void add_lasso_point(const glm::vec2& point);
    void end_lasso_selection();
    void cancel_lasso_selection();
    
    // Keyboard selection
    void enable_keyboard_selection(bool enabled);
    void move_keyboard_focus(Direction direction);
    void extend_keyboard_selection(Direction direction);
    void select_all_in_direction(Direction direction);
    
    entt::entity get_keyboard_focus() const { return keyboard_focus_entity_; }
    
    // Selection constraints
    void set_selection_constraints(SelectionType allowed_types);
    void lock_selection(bool locked);
    bool is_selection_locked() const { return lock_selection_; }
    
    // Selection operations
    void select_all_clips();
    void select_all_clips_on_track(entt::entity track);
    void select_clips_in_range(TimePoint start_time, TimePoint end_time);
    void select_clips_in_duration(Duration duration);
    
    void select_all_tracks();
    void select_tracks_of_type(Track::Type type);
    
    void select_all_markers();
    void select_markers_in_range(TimePoint start_time, TimePoint end_time);
    
    void select_all_keyframes();
    void select_keyframes_in_range(TimePoint start_time, TimePoint end_time);
    void select_keyframes_for_entity(entt::entity entity);
    
    // Selection by properties
    void select_clips_by_media_type(const std::string& media_type);
    void select_clips_by_duration(Duration min_duration, Duration max_duration);
    void select_clips_by_text(const std::string& search_text, bool case_sensitive = false);
    void select_clips_by_effect(const std::string& effect_id);
    
    // Selection history
    void save_selection_state(const std::string& description = "");
    void restore_selection_state(size_t history_index = 0);
    bool can_undo_selection() const;
    bool can_redo_selection() const;
    void undo_selection();
    void redo_selection();
    
    void clear_selection_history();
    
    // Selection manipulation
    void invert_selection(SelectionType type = SelectionType::All);
    void grow_selection(Duration amount);
    void shrink_selection(Duration amount);
    void extend_selection_to_start();
    void extend_selection_to_end();
    
    // Selection validation
    bool validate_selection() const;
    std::vector<std::string> get_selection_errors() const;
    
    // Callbacks
    void set_selection_changed_callback(std::function<void(const std::vector<entt::entity>&)> callback);
    void set_selection_count_callback(std::function<void(SelectionType, int)> callback);
    
private:
    void update_selection_history();
    void notify_selection_changed();
    void notify_selection_count_changed(SelectionType type, int count);
    
    bool is_entity_in_selection_region(entt::entity entity) const;
    bool is_point_in_lasso(const glm::vec2& point) const;
    
    entt::entity find_next_entity_in_direction(Direction direction) const;
    entt::entity find_prev_entity_in_direction(Direction direction) const;
    
    void clear_selection_internal();
    void add_to_selection_internal(entt::entity entity);
    void remove_from_selection_internal(entt::entity entity);
};

// Selection renderer
class SelectionRenderer {
private:
    TimelineManager* timeline_;
    SelectionManager* selection_manager_;
    
    // Rendering resources
    VkPipeline selection_pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    
    // Selection colors
    glm::vec4 primary_color_ = glm::vec4(0.2f, 0.4f, 0.8f, 1.0f);
    glm::vec4 secondary_color_ = glm::vec4(0.4f, 0.6f, 0.9f, 1.0f);
    glm::vec4 outline_color_ = glm::vec4(0.1f, 0.3f, 0.7f, 1.0f);
    
    float selection_thickness_ = 2.0f;
    float handle_size_ = 8.0f;
    
public:
    explicit SelectionRenderer(TimelineManager* timeline, SelectionManager* selection_manager);
    ~SelectionRenderer();
    
    bool initialize(VkDevice device, VkRenderPass render_pass);
    void cleanup();
    
    void render_selections(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_selection_handles(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_selection_region(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_lasso_selection(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    
    // Appearance
    void set_primary_color(const glm::vec4& color);
    void set_secondary_color(const glm::vec4& color);
    void set_outline_color(const glm::vec4& color);
    void set_selection_thickness(float thickness);
    void set_handle_size(float size);
    
private:
    bool create_render_pipeline(VkDevice device, VkRenderPass render_pass);
    void destroy_render_pipeline();
    
    void render_entity_selection(VkCommandBuffer command_buffer, entt::entity entity);
    void render_track_selection(VkCommandBuffer command_buffer, entt::entity track);
    void render_selection_handles_for_entity(VkCommandBuffer command_buffer, entt::entity entity);
};

} // namespace Shotcut::Timeline
```

---

## 7.7 ZOOM AND NAVIGATION CONTROLS

### 7.7.1 Professional Timeline Navigation

**Navigation Philosophy:**
- **Smooth Scrolling:** 60fps smooth scrolling and zooming
- **Professional Shortcuts:** JKL navigation, timeline shortcuts
- **Smart Zooming:** Zoom to selection, zoom to fit, zoom to cursor
- **Multiple View Modes:** Different zoom levels and view modes

**src/timeline/navigation.h:**
```cpp
#pragma once

#include "timeline_ecs.h"
#include <memory>
#include <vector>
#include <chrono>

namespace Shotcut::Timeline {

// Zoom levels and modes
enum class ZoomMode : uint8_t {
    Fit,            // Fit entire timeline
    Selection,      // Zoom to selection
    Cursor,         // Zoom to cursor/playhead
    Manual,         // Manual zoom level
    Adaptive        // Adaptive based on content
};

// View modes
enum class ViewMode : uint8_t {
    Normal,         // Standard timeline view
    Compact,        // Compact view with less detail
    Detailed,       // Detailed view with maximum information
    AudioFocused,   // Focus on audio tracks
    VideoFocused    // Focus on video tracks
};

// Navigation state
struct NavigationState {
    // Viewport
    glm::vec2 viewport_position = glm::vec2(0.0f);  // Top-left of viewport
    glm::vec2 viewport_size = glm::vec2(1920.0f, 1080.0f);
    
    // Time range visible in viewport
    TimePoint visible_start_time = Duration{0};
    TimePoint visible_end_time = Duration{0};
    Duration visible_duration = Duration{0};
    
    // Track range visible in viewport
    int visible_start_track = 0;
    int visible_end_track = 0;
    int visible_track_count = 0;
    
    // Zoom
    float pixels_per_second = 50.0f;
    float track_height = 50.0f;
    float min_zoom_level = 0.1f;
    float max_zoom_level = 1000.0f;
    
    // Animation
    bool animating = false;
    std::chrono::steady_clock::time_point animation_start;
    std::chrono::milliseconds animation_duration{300};
    
    // Constraints
    bool constrain_to_content = true;
    bool smooth_scrolling = true;
    bool snap_to_frame = false;
};

// Timeline navigation manager
class TimelineNavigation {
private:
    TimelineManager* timeline_;
    
    // Navigation state
    NavigationState state_;
    NavigationState target_state_;
    
    // Animation
    bool animation_enabled_ = true;
    std::chrono::milliseconds default_animation_duration_{300};
    std::function<void()> animation_callback_;
    
    // Zoom presets
    std::vector<float> zoom_presets_;
    size_t current_zoom_preset_ = 0;
    
    // Navigation history
    struct NavigationPosition {
        TimePoint center_time;
        float zoom_level;
        int center_track;
        ViewMode view_mode;
        std::chrono::system_clock::time_point timestamp;
    };
    
    std::vector<NavigationPosition> navigation_history_;
    size_t history_index_ = 0;
    size_t max_history_size_ = 50;
    
    // Keyboard navigation
    bool keyboard_navigation_enabled_ = true;
    std::chrono::milliseconds keyboard_scroll_speed_{50};
    float keyboard_zoom_speed_ = 1.1f;
    
public:
    explicit TimelineNavigation(TimelineManager* timeline);
    ~TimelineNavigation() = default;
    
    // Viewport management
    void set_viewport_size(const glm::vec2& size);
    void set_viewport_position(const glm::vec2& position);
    void set_visible_time_range(TimePoint start_time, TimePoint end_time);
    void set_visible_track_range(int start_track, int end_track);
    
    const NavigationState& get_state() const { return state_; }
    glm::vec2 get_viewport_position() const { return state_.viewport_position; }
    glm::vec2 get_viewport_size() const { return state_.viewport_size; }
    
    // Zoom operations
    void set_zoom_level(float pixels_per_second);
    void zoom_in(const glm::vec2& center_point = glm::vec2(-1.0f));
    void zoom_out(const glm::vec2& center_point = glm::vec2(-1.0f));
    void zoom_to_fit();
    void zoom_to_selection();
    void zoom_to_time(TimePoint time);
    void zoom_to_playhead();
    void zoom_to_cursor(const glm::vec2& cursor_position);
    
    // Zoom presets
    void next_zoom_preset();
    void previous_zoom_preset();
    void set_zoom_preset(size_t preset_index);
    void add_zoom_preset(float zoom_level);
    
    // Scrolling operations
    void scroll_horizontal(float delta_pixels);
    void scroll_vertical(float delta_pixels);
    void scroll_to_time(TimePoint time);
    void scroll_to_track(int track_index);
    void scroll_to_center(const glm::vec2& center_position);
    
    // Smooth scrolling
    void smooth_scroll_to(const glm::vec2& target_position);
    void smooth_scroll_to_time(TimePoint time);
    void smooth_scroll_to_track(int track_index);
    
    // View modes
    void set_view_mode(ViewMode mode);
    ViewMode get_view_mode() const;
    
    // JKL navigation (professional)
    void jkl_navigate(float speed_multiplier); // J: back, K: pause, L: forward
    void jkl_reverse(); // Shift+J for reverse
    void jkl_seek_forward(float multiplier = 1.0f);
    void jkl_seek_backward(float multiplier = 1.0f);
    
    // Frame navigation
    void next_frame();
    void previous_frame();
    void seek_to_frame(int64_t frame_number);
    
    // Clip navigation
    void next_clip();
    void previous_clip();
    void next_clip_on_track();
    void previous_clip_on_track();
    void goto_clip_start();
    void goto_clip_end();
    
    // Marker navigation
    void next_marker();
    void previous_marker();
    void goto_marker(const std::string& marker_name);
    void goto_next_marker_type(const std::string& type);
    
    // Cut point navigation
    void next_cut_point();
    void previous_cut_point();
    void add_cut_point_at_playhead();
    
    // Navigation history
    void save_navigation_position(const std::string& description = "");
    void restore_navigation_position(size_t history_index = 0);
    bool can_undo_navigation() const;
    bool can_redo_navigation() const;
    void undo_navigation();
    void redo_navigation();
    
    void clear_navigation_history();
    
    // Animation
    void enable_animation(bool enabled);
    void set_animation_duration(std::chrono::milliseconds duration);
    bool is_animating() const { return state_.animating; }
    
    // Coordinate conversion
    TimePoint screen_x_to_time(float x) const;
    float time_to_screen_x(TimePoint time) const;
    int screen_y_to_track(float y) const;
    float track_to_screen_y(int track_index) const;
    
    glm::vec2 time_track_to_screen(TimePoint time, int track_index) const;
    TimePoint screen_to_time(const glm::vec2& screen_position) const;
    int screen_to_track(const glm::vec2& screen_position) const;
    
    // Constraints
    void constrain_to_content(bool enabled);
    void enable_smooth_scrolling(bool enabled);
    void enable_snap_to_frame(bool enabled);
    
    // Performance
    void update_navigation();
    bool needs_redraw() const;
    
    // Validation
    bool validate_navigation_state() const;
    
private:
    // Animation helpers
    void start_animation();
    void update_animation();
    void finish_animation();
    
    // Zoom helpers
    void update_zoom_constraints();
    TimePoint get_optimal_time_range() const;
    std::pair<int, int> get_optimal_track_range() const;
    
    // Scrolling helpers
    void clamp_viewport_position();
    void update_visible_ranges();
    
    // Navigation helpers
    TimePoint get_playhead_position() const;
    entt::entity get_clip_at_time(TimePoint time) const;
    entt::entity get_marker_at_time(TimePoint time) const;
    
    // History helpers
    void save_current_position_to_history(const std::string& description);
    
    // Coordinate conversion helpers
    float calculate_pixels_per_second() const;
    float calculate_track_height() const;
};

// Navigation renderer
class NavigationRenderer {
private:
    TimelineManager* timeline_;
    TimelineNavigation* navigation_;
    
    // Rendering resources
    VkPipeline playhead_pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    
    // Playhead appearance
    glm::vec4 playhead_color_ = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    float playhead_thickness_ = 2.0f;
    float playhead_height_ = 100.0f;
    
    // Grid and guides
    bool show_grid_ = true;
    bool show_rulers_ = true;
    bool show_time_markers_ = true;
    glm::vec4 grid_color_ = glm::vec4(0.3f, 0.3f, 0.3f, 0.5f);
    glm::vec4 ruler_color_ = glm::vec4(0.5f, 0.5f, 0.5f, 0.8f);
    
    // Time display
    bool show_time_code_ = true;
    std::string time_format_ = "HH:MM:SS:FF";
    
public:
    explicit NavigationRenderer(TimelineManager* timeline, TimelineNavigation* navigation);
    ~NavigationRenderer();
    
    bool initialize(VkDevice device, VkRenderPass render_pass);
    void cleanup();
    
    void render_navigation_elements(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_playhead(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_time_ruler(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_track_ruler(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    void render_grid(VkCommandBuffer command_buffer, const glm::mat4& projection_matrix);
    
    // Appearance
    void set_playhead_color(const glm::vec4& color);
    void set_playhead_thickness(float thickness);
    void set_playhead_height(float height);
    
    void show_grid(bool show);
    void show_rulers(bool show);
    void show_time_markers(bool show);
    
    void set_grid_color(const glm::vec4& color);
    void set_ruler_color(const glm::vec4& color);
    
    void show_time_code(bool show);
    void set_time_format(const std::string& format);
    
private:
    bool create_render_pipeline(VkDevice device, VkRenderPass render_pass);
    void destroy_render_pipeline();
    
    std::string format_time_code(TimePoint time) const;
    void render_time_marker(VkCommandBuffer command_buffer, TimePoint time, const glm::mat4& projection_matrix);
    
    VkShaderModule create_vertex_shader();
    VkShaderModule create_fragment_shader();
};

} // namespace Shotcut::Timeline
```

---

## PHASE 7 IMPLEMENTATION ROADMAP

### 7.8 Month-by-Month Implementation Plan

**Month 11: Core Timeline System**
- Week 1-2: ECS timeline architecture implementation
- Week 3-4: Timeline serialization and persistence
- Week 5-6: Basic clip management system
- Week 7-8: Track system implementation

**Month 12: Professional Editing**
- Week 1-2: Basic editing operations (cut, trim, split)
- Week 3-4: Advanced editing operations (ripple, slip, slide)
- Week 5-6: Drag-and-drop functionality
- Week 7-8: Selection and manipulation tools

**Month 13: Navigation and UI**
- Week 1-2: Zoom and navigation controls
- Week 1-2: JKL navigation and professional shortcuts
- Week 5-6: Timeline rendering and display
- Week 7-8: Performance optimization and testing

**Month 14: Polish and Integration**
- Week 1-2: Timeline-UI integration
- Week 3-4: Advanced features and shortcuts
- Week 5-6: Performance profiling and optimization
- Week 7-8: Comprehensive testing and validation

### 7.9 Critical Success Metrics

**Performance Targets:**
- Timeline responsiveness: <16ms for all operations
- Clip manipulation: Smooth drag-and-drop with 1000+ clips
- Zoom performance: 60fps smooth zooming and panning
- Memory usage: <500MB for timelines with 1000 clips
- Selection performance: <1ms for selection operations

**Quality Targets:**
- Frame-accurate editing with microsecond precision
- Zero lag in clip manipulation
- Smooth animation and transitions
- Professional keyboard shortcuts (JKL navigation)
- Intuitive drag-and-drop with visual feedback

**Professional Features:**
- Non-destructive editing with effect stacks
- Ripple, slip, and slide editing modes
- Multi-track compositing with blend modes
- Keyframe automation for all parameters
- Professional navigation and workflow optimization

This Phase 7 implementation guide provides the foundation for building a professional-grade timeline and editing system that rivals commercial video editors, with modern C++20/23 ECS architecture, real-time performance, and comprehensive professional editing features.
