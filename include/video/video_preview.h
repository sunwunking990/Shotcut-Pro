#pragma once

#include "frame_buffer.h"
#include "ffmpeg_wrapper.h"
#include <memory>
#include <vector>
#include <chrono>
#include <functional>

namespace Shotcut::Video {

// Preview quality settings
enum class PreviewQuality : uint8_t {
    Draft,      // 1/4 resolution, effects disabled
    Low,        // 1/2 resolution, reduced effects
    Medium,      // 3/4 resolution, most effects
    High,        // Full resolution, all effects
    Auto         // Automatic based on performance
};

// Color space and conversion
enum class ColorSpace : uint8_t {
    SRGB,        // Standard RGB
    Rec601,      // SD video
    Rec709,      // HD video
    Rec2020,      // Ultra HD / HDR
    P3,          // Display P3
    Linear        // Linear color space
};

// Preview rendering pipeline
class VideoPreview {
private:
    // Device references
    VkDevice vulkan_device_;
    VkQueue vulkan_queue_;
    uint32_t vulkan_queue_family_;
    
    // Frame management
    std::unique_ptr<FramePool> frame_pool_;
    std::unique_ptr<FrameCache> frame_cache_;
    
    // Preview properties
    uint32_t preview_width_ = 1920;
    uint32_t preview_height_ = 1080;
    ColorSpace color_space_ = ColorSpace::Rec709;
    PreviewQuality quality_ = PreviewQuality::Auto;
    
    // Zoom and pan
    float zoom_level_ = 1.0f;
    struct {
        float x = 0.0f;
        float y = 0.0f;
    } pan_offset_;
    bool auto_fit_ = false;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    float current_fps_ = 0.0f;
    float target_fps_ = 60.0f;
    uint32_t frame_drops_ = 0;
    uint32_t total_frames_ = 0;
    
    // Decoder
    std::unique_ptr<FFmpegDecoder> decoder_;
    
    // Current frame
    std::shared_ptr<GPUFrame> current_frame_;
    std::mutex current_frame_mutex_;
    
    // Rendering state
    bool is_initialized_ = false;
    bool is_playing_ = false;
    
public:
    VideoPreview();
    ~VideoPreview();
    
    // Initialization
    bool initialize(VkDevice device, VkQueue queue, uint32_t queue_family);
    void cleanup();
    bool is_initialized() const { return is_initialized_; }
    
    // Preview configuration
    void set_resolution(uint32_t width, uint32_t height);
    void set_color_space(ColorSpace color_space);
    void set_quality(PreviewQuality quality);
    void set_target_fps(float fps);
    
    // Zoom and pan
    void set_zoom(float zoom);
    void set_pan(float x, float y);
    void set_auto_fit(bool auto_fit);
    void zoom_to_fit();
    void zoom_in(float center_x = 0.5f, float center_y = 0.5f);
    void zoom_out(float center_x = 0.5f, float center_y = 0.5f);
    
    // Media loading
    bool load_media(const std::string& file_path);
    void unload_media();
    bool is_media_loaded() const;
    
    // Rendering
    bool render_frame();
    bool render_frame_with_effects(const std::vector<std::string>& effect_chain);
    
    // Playback control
    void play();
    void pause();
    void stop();
    bool is_playing() const { return is_playing_; }
    
    // Frame access
    std::shared_ptr<GPUFrame> get_current_frame() const;
    bool seek_to_frame(int64_t frame_number);
    bool seek_to_time(int64_t time_microseconds);
    
    // Media information
    int get_media_width() const;
    int get_media_height() const;
    float get_media_aspect_ratio() const;
    int64_t get_media_duration() const;
    float get_media_framerate() const;
    int64_t get_current_frame_number() const;
    int64_t get_total_frame_count() const;
    
    // Performance
    float get_current_fps() const { return current_fps_; }
    uint32_t get_frame_drops() const { return frame_drops_; }
    float get_frame_drop_rate() const;
    
    void reset_performance_stats();
    
    // Quality management
    void auto_adjust_quality();
    PreviewQuality get_recommended_quality() const;
    
    // Callbacks
    void set_frame_rendered_callback(std::function<void(std::shared_ptr<GPUFrame>)> callback);
    void set_error_callback(std::function<void(const std::string&)> callback);
    
private:
    // Rendering helpers
    bool setup_rendering_pipeline();
    bool create_render_targets();
    void update_projection_matrix();
    void update_quality_settings();
    
    // Frame processing
    std::shared_ptr<GPUFrame> process_frame(std::shared_ptr<GPUFrame> input_frame);
    std::shared_ptr<GPUFrame> apply_effects(std::shared_ptr<GPUFrame> frame, 
                                           const std::vector<std::string>& effect_chain);
    
    // Performance tracking
    void update_performance_stats();
    void adjust_quality_for_performance();
    
    // Callbacks
    std::function<void(std::shared_ptr<GPUFrame>)> frame_rendered_callback_;
    std::function<void(const std::string&)> error_callback_;
    
    void notify_frame_rendered(std::shared_ptr<GPUFrame> frame);
    void notify_error(const std::string& error);
};

// Multi-output preview manager
class PreviewManager {
private:
    struct PreviewOutput {
        std::string name;
        std::unique_ptr<VideoPreview> preview;
        bool is_active;
        PreviewQuality quality_override;
        ColorSpace color_space_override;
    };
    
    std::vector<PreviewOutput> preview_outputs_;
    VkDevice vulkan_device_;
    uint32_t active_output_index_ = 0;
    
public:
    PreviewManager(VkDevice vulkan_device);
    ~PreviewManager();
    
    // Output management
    bool add_output(const std::string& name, VkQueue queue, uint32_t queue_family);
    void remove_output(const std::string& name);
    void set_active_output(const std::string& name);
    
    // Preview control
    bool render_to_all_outputs();
    bool render_to_active_output();
    
    // Media control (applies to all outputs)
    bool load_media_to_all_outputs(const std::string& file_path);
    void unload_media_from_all_outputs();
    
    // Output configuration
    void set_output_quality(const std::string& output_name, PreviewQuality quality);
    void set_output_color_space(const std::string& output_name, ColorSpace color_space);
    void set_output_resolution(const std::string& output_name, uint32_t width, uint32_t height);
    
    // Multi-display support
    std::vector<std::string> get_available_outputs() const;
    std::string get_active_output() const;
    
    // Performance
    std::vector<float> get_all_fps_metrics() const;
    float get_worst_fps() const;
    float get_best_fps() const;
    
private:
    PreviewOutput* find_output(const std::string& name);
};

} // namespace Shotcut::Video
