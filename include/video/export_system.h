#pragma once

#include "ffmpeg_wrapper.h"
#include "nvidia_video_sdk.h"
#include "frame_buffer.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>

namespace Shotcut::Video {

// Forward declarations for missing types
class FFmpegEncoder;
class HardwareDecoder;
class AudioBuffer;

enum class ColorSpace {
    Rec601,
    Rec709,
    Rec2020,
    DCI_P3,
    sRGB
};

enum class PixelFormat : uint8_t {
    UNKNOWN = 0,
    YUV420P,
    YUV444P,
    RGB24,
    RGBA,
    BGRA,
    NV12,
    P010,
    P016,
    CUDA,
    VULKAN,
    PCM_S16,
    PCM_S32,
    PCM_F32
};

// Export formats and codecs
enum class ExportFormat : uint8_t {
    MP4_H264,      // H.264 in MP4 container
    MP4_H265,      // H.265/HEVC in MP4 container
    MP4_AV1,        // AV1 in MP4 container
    MKV_H264,       // H.264 in MKV container
    MKV_H265,       // H.265 in MKV container
    WEBM_VP9,       // VP9 in WebM container
    WEBM_AV1,       // AV1 in WebM container
    MOV_PRORES,      // ProRes in MOV container
    MXF_DNXHD,      // DNxHD in MXF container
    MPEG_TS,        // MPEG-TS (broadcast)
    FLV,            // Flash Video (legacy)
    AVI,            // AVI container (legacy)
    GIF             // Animated GIF
};

// Export quality presets
enum class ExportQuality : uint8_t {
    Draft,          // Fast, low quality
    Low,            // Good quality, small file
    Medium,         // Balanced quality/size
    High,           // High quality
    Ultra,          // Maximum quality
    Lossless,       // No compression
    Custom          // User-defined settings
};

// Export presets for different platforms
enum class ExportPreset : uint8_t {
    Custom,         // User-defined
    
    // YouTube presets
    YouTube_1080p,      // 1080p H.264
    YouTube_4K,          // 4K H.265
    YouTube_720p,        // 720p H.264
    YouTube_480p,        // 480p H.264
    YouTube Shorts,       // Vertical 9:16 format
    
    // Vimeo presets
    Vimeo_1080p,         // 1080p H.264
    Vimeo_4K,            // 4K H.265
    Vimeo_720p,          // 720p H.264
    Vimeo_HD,            // HD H.264
    
    // Broadcast presets
    Broadcast_1080i,     // 1080i broadcast
    Broadcast_720p,      // 720p broadcast
    Broadcast_4K,         // 4K broadcast
    
    // Professional presets
    ProRes_422,          // ProRes 422 HQ
    ProRes_444,          // ProRes 4444
    DNxHR_444,          // DNxHR 444
    Cinema_DCP,          // Digital Cinema Package
    
    // Social media presets
    Instagram_1080p,     // Square format
    TikTok_1080p,        // Vertical 9:16
    Twitter_1080p,       // Optimized for Twitter
    Facebook_1080p,       // Optimized for Facebook
    
    // Device presets
    iPhone_1080p,        // Optimized for iPhone
    Android_1080p,        // Optimized for Android
    iPad_4K,             // Optimized for iPad
    
    // Web presets
    Web_1080p,           // Web streaming
    Web_720p,            // Web streaming lower quality
    Adaptive_Streaming     // Adaptive bitrate streaming
};

// Export configuration
struct ExportConfig {
    // Basic settings
    ExportFormat format = ExportFormat::MP4_H264;
    ExportQuality quality = ExportQuality::High;
    ExportPreset preset = ExportPreset::Custom;
    
    // Video settings
    int width = 1920;
    int height = 1080;
    float frame_rate = 30.0f;
    int64_t bitrate = 5000000; // 5 Mbps
    PixelFormat pixel_format = PixelFormat::YUV420P;
    
    // Audio settings
    bool include_audio = true;
    int audio_sample_rate = 48000;
    int audio_channels = 2;
    int64_t audio_bitrate = 128000; // 128 kbps
    PixelFormat audio_format = PixelFormat::PCM_S16;
    
    // Advanced video settings
    int gop_size = 30; // GOP length
    int b_frames = 0; // B-frame count
    int profile = -1; // H.264/H.265 profile
    int level = -1; // H.264/H.265 level
    bool two_pass = false;
    bool cabac = true; // H.264 CABAC
    bool deblock = true; // Deblocking filter
    
    // Hardware acceleration
    bool use_hardware_acceleration = true;
    std::string hardware_device = "auto"; // auto, cuda, vaapi, etc.
    
    // Color and metadata
    ColorSpace color_space = ColorSpace::Rec709;
    bool include_metadata = true;
    bool embed_chapters = true;
    bool embed_subtitles = true;
    
    // Output settings
    std::string output_path;
    bool overwrite_existing = false;
    bool create_thumbnail = true;
    
    // Performance settings
    int thread_count = 0; // 0 = auto
    size_t buffer_size = 64 * 1024 * 1024; // 64MB
    bool low_latency = false;
    
    // Validation and repair
    bool validate_output = true;
    bool auto_repair = false;
    
    static ExportConfig from_preset(ExportPreset preset);
    ExportConfig validate() const;
    bool is_valid() const;
};

// Export progress information
struct ExportProgress {
    uint64_t frames_processed = 0;
    uint64_t total_frames = 0;
    float percentage = 0.0f;
    float fps = 0.0f;
    float bitrate = 0.0f;
    std::chrono::milliseconds estimated_time_remaining{0};
    std::chrono::milliseconds elapsed_time{0};
    
    // Current operation
    std::string current_operation;
    std::string current_file;
    
    // Error information
    bool has_error = false;
    std::string error_message;
    
    // Performance metrics
    float cpu_usage = 0.0f;
    float gpu_usage = 0.0f;
    size_t memory_usage = 0;
    
    void update(uint64_t processed, uint64_t total, 
               const std::chrono::steady_clock::time_point& start_time);
    void set_error(const std::string& error);
};

// Export job for batch processing
struct ExportJob {
    std::string id;
    ExportConfig config;
    std::string input_timeline_path;
    std::string output_path;
    int priority = 0; // Higher number = higher priority
    
    // Job status
    enum class Status {
        Queued,
        Processing,
        Completed,
        Failed,
        Cancelled
    } status = Status::Queued;
    
    // Progress tracking
    ExportProgress progress;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point end_time;
    
    // Dependencies
    std::vector<std::string> dependencies; // Other job IDs that must complete first
    std::function<void(const ExportProgress&)> progress_callback;
    std::function<void(bool)> completion_callback;
};

// Main export system
class ExportSystem {
private:
    // Active export session
    struct ExportSession {
        std::unique_ptr<FFmpegEncoder> encoder;
        std::unique_ptr<NVENCEncoder> nvenc_encoder;
        std::unique_ptr<HardwareDecoder> decoder;
        
        ExportConfig config;
        ExportProgress progress;
        
        // Threading
        std::thread export_thread;
        std::atomic<bool> should_cancel{false};
        std::atomic<bool> is_exporting{false};
        
        // Timing
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_progress_update;
        
        // Performance tracking
        uint64_t frames_processed = 0;
        uint64_t frames_total = 0;
        double average_fps = 0.0f;
        size_t bytes_written = 0;
        
        // Synchronization
        std::mutex progress_mutex;
        std::condition_variable progress_cv;
    };
    
    std::unique_ptr<ExportSession> current_session_;
    
    // Batch export management
    struct BatchExportManager {
        std::queue<std::unique_ptr<ExportJob>> job_queue;
        std::vector<std::unique_ptr<ExportJob>> active_jobs;
        std::vector<std::unique_ptr<ExportJob>> completed_jobs;
        
        // Threading
        std::vector<std::thread> worker_threads;
        std::atomic<bool> shutdown_requested{false};
        std::mutex queue_mutex;
        std::condition_variable queue_cv;
        
        // Configuration
        size_t max_concurrent_jobs = 2;
        size_t max_completed_jobs = 100;
        
        void worker_function();
        void process_job(std::unique_ptr<ExportJob> job);
        void cleanup_completed_jobs();
    };
    
    std::unique_ptr<BatchExportManager> batch_manager_;
    
    // Preset management
    struct PresetManager {
        std::unordered_map<ExportPreset, ExportConfig> presets;
        std::vector<ExportConfig> custom_presets;
        
        void load_builtin_presets();
        void load_custom_presets();
        void save_custom_presets();
    };
    
    std::unique_ptr<PresetManager> preset_manager_;
    
    // Hardware capability detection
    struct HardwareCapabilities {
        bool nvenc_available = false;
        bool nvdec_available = false;
        bool vaapi_available = false;
        bool videotoolbox_available = false;
        std::vector<int> cuda_devices;
        
        void detect_capabilities();
    };
    
    HardwareCapabilities hw_caps_;
    
    // Export statistics
    struct ExportStatistics {
        uint64_t total_exports = 0;
        uint64_t successful_exports = 0;
        uint64_t failed_exports = 0;
        std::chrono::milliseconds total_export_time{0};
        std::chrono::milliseconds average_export_time{0};
        std::unordered_map<ExportFormat, uint64_t> format_usage;
        std::unordered_map<ExportPreset, uint64_t> preset_usage;
    };
    
    ExportStatistics stats_;
    
public:
    ExportSystem();
    ~ExportSystem();
    
    // Initialization
    bool initialize();
    void cleanup();
    bool is_initialized() const;
    
    // Single export
    bool start_export(const ExportConfig& config, 
                   std::function<void(const ExportProgress&)> progress_callback = nullptr,
                   std::function<void(bool)> completion_callback = nullptr);
    void cancel_export();
    bool is_exporting() const;
    ExportProgress get_progress() const;
    
    // Batch export
    std::string add_batch_job(const ExportJob& job);
    bool remove_batch_job(const std::string& job_id);
    std::vector<std::string> get_batch_job_ids() const;
    ExportJob* get_batch_job(const std::string& job_id);
    
    void start_batch_export();
    void pause_batch_export();
    void resume_batch_export();
    void cancel_batch_export();
    
    // Preset management
    ExportConfig get_preset(ExportPreset preset) const;
    std::vector<ExportPreset> get_available_presets() const;
    std::vector<ExportConfig> get_custom_presets() const;
    
    bool add_custom_preset(const std::string& name, const ExportConfig& config);
    bool remove_custom_preset(const std::string& name);
    bool update_custom_preset(const std::string& name, const ExportConfig& config);
    
    // Hardware capabilities
    bool is_nvenc_available() const { return hw_caps_.nvenc_available; }
    bool is_nvdec_available() const { return hw_caps_.nvdec_available; }
    bool is_vaapi_available() const { return hw_caps_.vaapi_available; }
    std::vector<int> get_cuda_devices() const { return hw_caps_.cuda_devices; }
    
    // Validation and testing
    bool validate_export_config(const ExportConfig& config) const;
    bool test_export_settings(const ExportConfig& config);
    std::vector<std::string> get_validation_warnings(const ExportConfig& config) const;
    
    // Statistics
    const ExportStatistics& get_statistics() const { return stats_; }
    void reset_statistics();
    void export_statistics(const std::string& file_path) const;
    void import_statistics(const std::string& file_path);
    
    // Configuration
    void set_max_concurrent_batch_jobs(size_t count);
    size_t get_max_concurrent_batch_jobs() const;
    
    void set_export_buffer_size(size_t size_bytes);
    size_t get_export_buffer_size() const;
    
private:
    // Export session management
    bool create_export_session(const ExportConfig& config);
    void cleanup_export_session();
    
    // Export implementation
    bool export_timeline(const ExportConfig& config);
    bool export_with_ffmpeg(const ExportConfig& config);
    bool export_with_nvenc(const ExportConfig& config);
    
    // Frame processing
    bool process_frame(std::shared_ptr<GPUFrame> frame, ExportSession* session);
    bool apply_effects_to_frame(std::shared_ptr<GPUFrame> frame, const ExportConfig& config);
    
    // Audio processing
    bool process_audio_frame(const AudioBuffer& audio_buffer, ExportSession* session);
    
    // Progress tracking
    void update_progress(ExportSession* session);
    void update_statistics(bool success, std::chrono::milliseconds export_time);
    
    // Error handling
    void handle_export_error(const std::string& error, ExportSession* session);
    bool recover_from_export_error(ExportSession* session);
    
    // Configuration helpers
    ExportConfig optimize_config_for_hardware(const ExportConfig& config);
    ExportConfig adapt_config_for_format(const ExportConfig& config, ExportFormat format);
    
    // Preset helpers
    void load_youtube_presets();
    void load_vimeo_presets();
    void load_broadcast_presets();
    void load_professional_presets();
    void load_social_media_presets();
    void load_device_presets();
    void load_web_presets();
};

// Export system singleton
class ExportManager {
private:
    static std::unique_ptr<ExportSystem> instance_;
    static std::mutex instance_mutex_;
    
public:
    static ExportSystem& get() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = std::make_unique<ExportSystem>();
            instance_->initialize();
        }
        return *instance_;
    }
    
    static void shutdown() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (instance_) {
            instance_->cleanup();
            instance_.reset();
        }
    }
};

// Utility functions
namespace ExportUtils {
    // Format utilities
    std::string format_to_string(ExportFormat format);
    std::string quality_to_string(ExportQuality quality);
    std::string preset_to_string(ExportPreset preset);
    
    ExportFormat string_to_format(const std::string& str);
    ExportQuality string_to_quality(const std::string& str);
    ExportPreset string_to_preset(const std::string& str);
    
    // File utilities
    std::string get_file_extension(ExportFormat format);
    std::string generate_output_filename(const std::string& input_path, 
                                    ExportFormat format, 
                                    const std::string& suffix = "");
    bool ensure_output_directory(const std::string& file_path);
    uint64_t get_available_disk_space(const std::string& path);
    
    // Quality utilities
    int64_t calculate_bitrate(int width, int height, float fps, ExportQuality quality);
    int calculate_optimal_gop_size(float fps);
    int calculate_optimal_buffer_size(int width, int height, ExportFormat format);
    
    // Validation utilities
    bool is_resolution_valid_for_format(ExportFormat format, int width, int height);
    bool is_framerate_valid_for_format(ExportFormat format, float fps);
    bool is_bitrate_valid_for_format(ExportFormat format, int64_t bitrate);
    
    // Performance utilities
    std::chrono::milliseconds estimate_export_time(
        int64_t total_frames, 
        float fps, 
        ExportFormat format,
        bool hardware_acceleration
    );
    
    size_t estimate_output_size(
        int64_t duration_ms,
        int64_t bitrate,
        bool include_audio,
        int64_t audio_bitrate
    );
}

} // namespace Shotcut::Video
