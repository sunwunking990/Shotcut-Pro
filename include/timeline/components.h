#pragma once

// Timeline Components - Entity Component System Components
// Professional video editing components for clips, effects, and timeline data

#include "entity.h"
#include <chrono>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace Shotcut::Timeline {

// Simple vector types for timeline components
struct Vec2 {
    float x, y;
    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x, float y) : x(x), y(y) {}
};

struct Vec3 {
    float x, y, z;
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

// ============================================================================
// CORE CLIP COMPONENTS
// ============================================================================

// Base clip information - common to all clip types
struct ClipComponent : public Component {
    std::string name;
    std::string file_path;
    
    // Timeline positioning
    double timeline_start_time;  // Position on timeline in seconds
    double duration;             // Clip duration in seconds
    double start_time;          // Start time within source media
    
    // Track information
    int track_id;               // Which track this clip is on (-1 for none)
    int track_index;            // Order within track
    
    // Clip state
    bool is_enabled;
    bool is_locked;
    bool is_selected;
    
    ClipComponent() 
        : timeline_start_time(0.0)
        , duration(0.0)
        , start_time(0.0)
        , track_id(-1)
        , track_index(0)
        , is_enabled(true)
        , is_locked(false)
        , is_selected(false) {}
    
    virtual const char* get_name() const override { return "ClipComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<ClipComponent>(); 
    }
    
    // Helper methods
    double get_end_time() const { return timeline_start_time + duration; }
    bool overlaps(double time) const {
        return time >= timeline_start_time && time < get_end_time();
    }
    bool overlaps(const ClipComponent& other) const {
        return track_id == other.track_id && 
               !(get_end_time() <= other.timeline_start_time || 
                 timeline_start_time >= other.get_end_time());
    }
};

// Video-specific clip data
struct VideoClipComponent : public Component {
    // Video properties
    int width;
    int height;
    double frame_rate;
    std::string codec;
    uint64_t bitrate;
    
    // Color information
    std::string color_space;
    std::string color_range;
    int bit_depth;
    
    // Thumbnail data
    std::vector<uint8_t> thumbnail_data;
    int thumbnail_width;
    int thumbnail_height;
    
    // Proxy information
    bool has_proxy;
    std::string proxy_path;
    bool use_proxy;
    
    VideoClipComponent()
        : width(1920)
        , height(1080)
        , frame_rate(30.0)
        , bitrate(0)
        , color_space("Rec709")
        , color_range("Limited")
        , bit_depth(8)
        , thumbnail_width(160)
        , thumbnail_height(90)
        , has_proxy(false)
        , use_proxy(false) {}
    
    virtual const char* get_name() const override { return "VideoClipComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<VideoClipComponent>(); 
    }
    
    // Helper methods
    Vec2 get_resolution() const { return Vec2(static_cast<float>(width), static_cast<float>(height)); }
    double get_aspect_ratio() const { return static_cast<double>(width) / height; }
};

// Audio-specific clip data
struct AudioClipComponent : public Component {
    // Audio properties
    int sample_rate;
    int channels;
    int bit_depth;
    uint64_t bitrate;
    std::string codec;
    
    // Audio levels
    float peak_level;
    float rms_level;
    std::vector<float> waveformData;  // Computed waveform data
    
    // Audio processing
    bool is_muted;
    float volume;               // 0.0 to 1.0
    float pan;                  // -1.0 (left) to 1.0 (right)
    
    AudioClipComponent()
        : sample_rate(48000)
        , channels(2)
        , bit_depth(16)
        , bitrate(0)
        , peak_level(0.0f)
        , rms_level(0.0f)
        , is_muted(false)
        , volume(1.0f)
        , pan(0.0f) {}
    
    virtual const char* get_name() const override { return "AudioClipComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<AudioClipComponent>(); 
    }
    
    // Helper methods
    bool is_stereo() const { return channels >= 2; }
    float get_db_level() const { return 20.0f * std::log10(rms_level + 1e-10f); }
};

// ============================================================================
// TRANSFORM COMPONENTS
// ============================================================================

// 2D transformation for video clips
struct TransformComponent : public Component {
    // Position and scale
    Vec2 position;
    Vec2 scale;
    float rotation;             // In degrees
    
    // Anchor point for transformations
    Vec2 anchor;               // Normalized 0.0-1.0
    Vec2 anchor_offset;         // Pixel offset
    
    // Opacity and blending
    float opacity;
    std::string blend_mode;
    
    TransformComponent()
        : position(0.0f, 0.0f)
        , scale(1.0f, 1.0f)
        , rotation(0.0f)
        , anchor(0.5f, 0.5f)
        , anchor_offset(0.0f, 0.0f)
        , opacity(1.0f)
        , blend_mode("Normal") {}
    
    virtual const char* get_name() const override { return "TransformComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<TransformComponent>(); 
    }
};

// Motion tracking data
struct MotionTrackingComponent : public Component {
    struct TrackingPoint {
        Vec2 position;
        float confidence;
        double timestamp;
    };
    
    std::vector<TrackingPoint> tracking_data;
    bool is_tracking_enabled;
    int track_point_count;
    
    MotionTrackingComponent()
        : is_tracking_enabled(false)
        , track_point_count(1) {}
    
    virtual const char* get_name() const override { return "MotionTrackingComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<MotionTrackingComponent>(); 
    }
    
    Vec2 get_position_at(double time) const {
        if (tracking_data.empty()) return Vec2(0.0f, 0.0f);
        
        // Linear interpolation between tracking points
        for (size_t i = 1; i < tracking_data.size(); ++i) {
            if (time <= tracking_data[i].timestamp) {
                const auto& p1 = tracking_data[i-1];
                const auto& p2 = tracking_data[i];
                
                double t = (time - p1.timestamp) / (p2.timestamp - p1.timestamp);
                return Vec2(
                    p1.position.x + (p2.position.x - p1.position.x) * static_cast<float>(t),
                    p1.position.y + (p2.position.y - p1.position.y) * static_cast<float>(t)
                );
            }
        }
        
        return tracking_data.back().position;
    }
};

// ============================================================================
// EFFECT COMPONENTS
// ============================================================================

// Base effect component
struct EffectComponent : public Component {
    std::string effect_name;
    std::string effect_type;    // "video", "audio", "transition"
    bool is_enabled;
    double start_time;
    double duration;
    
    // Effect parameters (key-value pairs)
    std::unordered_map<std::string, std::string> parameters;
    
    EffectComponent()
        : is_enabled(true)
        , start_time(0.0)
        , duration(0.0) {}
    
    virtual const char* get_name() const override { return "EffectComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<EffectComponent>(); 
    }
    
    template<typename T>
    void set_parameter(const std::string& name, const T& value) {
        if constexpr (std::is_same_v<T, std::string>) {
            parameters[name] = value;
        } else {
            parameters[name] = std::to_string(value);
        }
    }
    
    template<typename T>
    T get_parameter(const std::string& name, const T& default_value) const {
        auto it = parameters.find(name);
        if (it == parameters.end()) return default_value;
        
        if constexpr (std::is_same_v<T, std::string>) {
            return it->second;
        } else if constexpr (std::is_same_v<T, float>) {
            return std::stof(it->second);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(it->second);
        } else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(it->second);
        } else if constexpr (std::is_same_v<T, bool>) {
            return it->second == "true" || it->second == "1";
        }
        
        return default_value;
    }
};

// Color correction effect
struct ColorCorrectionComponent : public Component {
    // Primary color adjustments
    Vec3 brightness;      // RGB brightness
    Vec3 contrast;         // RGB contrast
    Vec3 saturation;       // RGB saturation
    Vec3 gamma;            // RGB gamma
    
    // Secondary color adjustments
    float hue_shift;
    float vibrance;
    
    // Lift/gamma/gain model
    Vec3 lift;
    Vec3 gamma_lift;
    Vec3 gain;
    
    ColorCorrectionComponent()
        : brightness(0.0f, 0.0f, 0.0f)
        , contrast(1.0f, 1.0f, 1.0f)
        , saturation(1.0f, 1.0f, 1.0f)
        , gamma(1.0f, 1.0f, 1.0f)
        , hue_shift(0.0f)
        , vibrance(0.0f)
        , lift(0.0f, 0.0f, 0.0f)
        , gamma_lift(1.0f, 1.0f, 1.0f)
        , gain(1.0f, 1.0f, 1.0f) {}
    
    virtual const char* get_name() const override { return "ColorCorrectionComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<ColorCorrectionComponent>(); 
    }
};

// Audio filter component
struct AudioFilterComponent : public Component {
    std::string filter_type;    // "equalizer", "compressor", "reverb", etc.
    
    // Common parameters
    float frequency;            // Hz
    float gain;               // dB
    float q_factor;            // Quality factor
    
    // Filter-specific parameters
    std::unordered_map<std::string, float> parameters;
    
    AudioFilterComponent()
        : frequency(1000.0f)
        , gain(0.0f)
        , q_factor(1.0f) {}
    
    virtual const char* get_name() const override { return "AudioFilterComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<AudioFilterComponent>(); 
    }
};

// ============================================================================
// TIMELINE COMPONENTS
// ============================================================================

// Timeline track information
struct TrackComponent : public Component {
    enum class Type {
        VIDEO,
        AUDIO,
        SUBTITLE,
        EFFECT
    };
    
    Type track_type;
    std::string track_name;
    int track_id;
    bool is_enabled;
    bool is_locked;
    bool is_solo;
    bool is_muted;
    
    // Track properties
    float height;              // Track height in pixels
    Vec4 color;               // Track color (RGBA)
    
    // Audio track specific
    float volume;
    float pan;
    
    TrackComponent(Type type = Type::VIDEO)
        : track_type(type)
        , track_id(-1)
        , is_enabled(true)
        , is_locked(false)
        , is_solo(false)
        , is_muted(false)
        , height(100.0f)
        , color(1.0f, 1.0f, 1.0f, 1.0f)
        , volume(1.0f)
        , pan(0.0f) {}
    
    virtual const char* get_name() const override { return "TrackComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<TrackComponent>(); 
    }
    
    bool is_video_track() const { return track_type == Type::VIDEO; }
    bool is_audio_track() const { return track_type == Type::AUDIO; }
};

// Timeline markers and regions
struct MarkerComponent : public Component {
    enum class Type {
        MARKER,
        REGION_START,
        REGION_END
    };
    
    Type marker_type;
    std::string name;
    std::string comment;
    double time;
    Vec4 color;
    
    MarkerComponent()
        : marker_type(Type::MARKER)
        , time(0.0)
        , color(1.0f, 0.0f, 0.0f, 1.0f) {} // Red default
    
    virtual const char* get_name() const override { return "MarkerComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<MarkerComponent>(); 
    }
};

// ============================================================================
// METADATA COMPONENTS
// ============================================================================

// File metadata component
struct MetadataComponent : public Component {
    std::unordered_map<std::string, std::string> metadata;
    
    // Common metadata fields
    std::string title;
    std::string artist;
    std::string album;
    std::string date;
    std::string description;
    std::string copyright;
    
    virtual const char* get_name() const override { return "MetadataComponent"; }
    virtual ComponentTypeID get_type_id() const override { 
        return ComponentTypeRegistry::get_type_id<MetadataComponent>(); 
    }
    
    void set_field(const std::string& key, const std::string& value) {
        metadata[key] = value;
    }
    
    std::string get_field(const std::string& key) const {
        auto it = metadata.find(key);
        return it != metadata.end() ? it->second : "";
    }
};

} // namespace Shotcut::Timeline
