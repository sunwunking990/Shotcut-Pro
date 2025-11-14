// Export System Implementation
// Professional video export with hardware acceleration and batch processing

#include "video/export_system.h"
#include "timeline/entity.h"
#include "effects/effect_system.h"
#include "audio/audio_processor.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <filesystem>

namespace Shotcut::Video {

// ============================================================================
// EXPORT PROGRESS IMPLEMENTATION
// ============================================================================

void ExportProgress::update(uint64_t processed, uint64_t total, 
                         const std::chrono::steady_clock::time_point& start_time) {
    frames_processed = processed;
    total_frames = total;
    
    if (total > 0) {
        percentage = static_cast<float>(processed) / static_cast<float>(total) * 100.0f;
    }
    
    auto now = std::chrono::steady_clock::now();
    elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    
    if (processed > 0 && elapsed_time.count() > 0) {
        fps = static_cast<float>(processed) / (elapsed_time.count() / 1000.0f);
        
        auto estimated_total_ms = (elapsed_time.count() * total) / processed;
        estimated_time_remaining = std::chrono::milliseconds(
            static_cast<int64_t>(estimated_total_ms - elapsed_time.count())
        );
    }
}

void ExportProgress::set_error(const std::string& error) {
    has_error = true;
    error_message = error;
}

// ============================================================================
// EXPORT CONFIG IMPLEMENTATION
// ============================================================================

ExportConfig ExportConfig::from_preset(ExportPreset preset) {
    ExportConfig config;
    config.preset = preset;
    
    switch (preset) {
        case ExportPreset::YouTube_1080p:
            config.format = ExportFormat::MP4_H264;
            config.width = 1920;
            config.height = 1080;
            config.frame_rate = 30.0f;
            config.bitrate = 8000000; // 8 Mbps
            config.audio_bitrate = 192000; // 192 kbps
            config.gop_size = 60;
            break;
            
        case ExportPreset::YouTube_4K:
            config.format = ExportFormat::MP4_H265;
            config.width = 3840;
            config.height = 2160;
            config.frame_rate = 30.0f;
            config.bitrate = 45000000; // 45 Mbps
            config.audio_bitrate = 384000; // 384 kbps
            config.gop_size = 60;
            break;
            
        case ExportPreset::YouTube_720p:
            config.format = ExportFormat::MP4_H264;
            config.width = 1280;
            config.height = 720;
            config.frame_rate = 30.0f;
            config.bitrate = 5000000; // 5 Mbps
            config.audio_bitrate = 128000; // 128 kbps
            config.gop_size = 60;
            break;
            
        case ExportPreset::YouTube_Shorts:
            config.format = ExportFormat::MP4_H264;
            config.width = 1080;
            config.height = 1920; // Vertical
            config.frame_rate = 30.0f;
            config.bitrate = 6000000; // 6 Mbps
            config.audio_bitrate = 128000; // 128 kbps
            config.gop_size = 30;
            break;
            
        case ExportPreset::Vimeo_1080p:
            config.format = ExportFormat::MP4_H264;
            config.width = 1920;
            config.height = 1080;
            config.frame_rate = 30.0f;
            config.bitrate = 10000000; // 10 Mbps
            config.audio_bitrate = 320000; // 320 kbps
            config.gop_size = 30;
            config.two_pass = true;
            break;
            
        case ExportPreset::Vimeo_4K:
            config.format = ExportFormat::MP4_H265;
            config.width = 3840;
            config.height = 2160;
            config.frame_rate = 30.0f;
            config.bitrate = 50000000; // 50 Mbps
            config.audio_bitrate = 384000; // 384 kbps
            config.gop_size = 30;
            config.two_pass = true;
            break;
            
        case ExportPreset::ProRes_422:
            config.format = ExportFormat::MOV_PRORES;
            config.width = 1920;
            config.height = 1080;
            config.frame_rate = 30.0f;
            config.bitrate = 145000000; // ProRes 422 HQ ~145 Mbps
            config.pixel_format = PixelFormat::YUV422P;
            config.use_hardware_acceleration = false; // ProRes usually better with CPU
            break;
            
        case ExportPreset::Instagram_1080p:
            config.format = ExportFormat::MP4_H264;
            config.width = 1080;
            config.height = 1080; // Square
            config.frame_rate = 30.0f;
            config.bitrate = 8000000; // 8 Mbps
            config.audio_bitrate = 128000;
            config.gop_size = 30;
            break;
            
        case ExportPreset::TikTok_1080p:
            config.format = ExportFormat::MP4_H264;
            config.width = 1080;
            config.height = 1920; // Vertical
            config.frame_rate = 30.0f;
            config.bitrate = 8000000; // 8 Mbps
            config.audio_bitrate = 128000;
            config.gop_size = 30;
            break;
            
        case ExportPreset::Web_1080p:
            config.format = ExportFormat::MP4_H264;
            config.width = 1920;
            config.height = 1080;
            config.frame_rate = 30.0f;
            config.bitrate = 5000000; // 5 Mbps
            config.audio_bitrate = 128000;
            config.gop_size = 60;
            config.low_latency = true;
            break;
            
        default:
            break; // Use default values for Custom
    }
    
    return config;
}

ExportConfig ExportConfig::validate() const {
    ExportConfig validated = *this;
    
    // Validate dimensions
    if (width <= 0 || height <= 0) {
        validated.width = 1920;
        validated.height = 1080;
    }
    
    // Validate frame rate
    if (frame_rate <= 0.0f || frame_rate > 240.0f) {
        validated.frame_rate = 30.0f;
    }
    
    // Validate bitrate
    if (bitrate <= 0) {
        validated.bitrate = ExportUtils::calculate_bitrate(width, height, frame_rate, quality);
    }
    
    // Validate audio settings
    if (audio_sample_rate <= 0) {
        validated.audio_sample_rate = 48000;
    }
    if (audio_channels <= 0 || audio_channels > 8) {
        validated.audio_channels = 2;
    }
    if (audio_bitrate <= 0) {
        validated.audio_bitrate = 128000;
    }
    
    // Validate GOP size
    if (gop_size <= 0) {
        validated.gop_size = ExportUtils::calculate_optimal_gop_size(frame_rate);
    }
    
    return validated;
}

bool ExportConfig::is_valid() const {
    return width > 0 && height > 0 && 
           frame_rate > 0.0f && frame_rate <= 240.0f &&
           bitrate > 0 && 
           !output_path.empty();
}

// ============================================================================
// EXPORT SYSTEM IMPLEMENTATION
// ============================================================================

ExportSystem::ExportSystem() {
    preset_manager_ = std::make_unique<PresetManager>();
    batch_manager_ = std::make_unique<BatchExportManager>();
}

ExportSystem::~ExportSystem() {
    cleanup();
}

bool ExportSystem::initialize() {
    // Detect hardware capabilities
    hw_caps_.detect_capabilities();
    
    // Load presets
    preset_manager_->load_builtin_presets();
    preset_manager_->load_custom_presets();
    
    // Initialize batch manager
    batch_manager_->worker_threads.reserve(batch_manager_->max_concurrent_jobs);
    for (size_t i = 0; i < batch_manager_->max_concurrent_jobs; ++i) {
        batch_manager_->worker_threads.emplace_back(&BatchExportManager::worker_function, batch_manager_.get());
    }
    
    return true;
}

void ExportSystem::cleanup() {
    // Cancel any active export
    cancel_export();
    
    // Shutdown batch manager
    if (batch_manager_) {
        batch_manager_->shutdown_requested = true;
        batch_manager_->queue_cv.notify_all();
        
        for (auto& thread : batch_manager_->worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        batch_manager_->worker_threads.clear();
    }
    
    cleanup_export_session();
}

bool ExportSystem::is_initialized() const {
    return preset_manager_ != nullptr && batch_manager_ != nullptr;
}

bool ExportSystem::start_export(const ExportConfig& config, 
                             std::function<void(const ExportProgress&)> progress_callback,
                             std::function<void(bool)> completion_callback) {
    if (is_exporting()) {
        return false;
    }
    
    // Validate configuration
    auto validated_config = config.validate();
    if (!validated_config.is_valid()) {
        return false;
    }
    
    // Create export session
    if (!create_export_session(validated_config)) {
        return false;
    }
    
    // Start export thread
    current_session_->is_exporting = true;
    current_session_->should_cancel = false;
    current_session_->start_time = std::chrono::steady_clock::now();
    
    current_session_->export_thread = std::thread([this, validated_config, progress_callback, completion_callback]() {
        bool success = export_timeline(validated_config);
        
        // Call completion callback
        if (completion_callback) {
            completion_callback(success);
        }
        
        // Cleanup session
        current_session_->is_exporting = false;
    });
    
    return true;
}

void ExportSystem::cancel_export() {
    if (current_session_ && current_session_->is_exporting) {
        current_session_->should_cancel = true;
        
        if (current_session_->export_thread.joinable()) {
            current_session_->export_thread.join();
        }
        
        cleanup_export_session();
    }
}

bool ExportSystem::is_exporting() const {
    return current_session_ && current_session_->is_exporting;
}

ExportProgress ExportSystem::get_progress() const {
    if (!current_session_) {
        return ExportProgress{};
    }
    
    std::lock_guard<std::mutex> lock(current_session_->progress_mutex);
    return current_session_->progress;
}

std::string ExportSystem::add_batch_job(const ExportJob& job) {
    if (!batch_manager_) {
        return "";
    }
    
    auto new_job = std::make_unique<ExportJob>(job);
    
    // Generate unique ID
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "export_";
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << dis(gen);
    }
    new_job->id = ss.str();
    
    // Add to queue
    {
        std::lock_guard<std::mutex> lock(batch_manager_->queue_mutex);
        batch_manager_->job_queue.push(std::move(new_job));
    }
    batch_manager_->queue_cv.notify_one();
    
    return new_job->id;
}

bool ExportSystem::remove_batch_job(const std::string& job_id) {
    if (!batch_manager_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(batch_manager_->queue_mutex);
    
    // Check active jobs
    auto it = std::find_if(batch_manager_->active_jobs.begin(), batch_manager_->active_jobs.end(),
                         [&job_id](const std::unique_ptr<ExportJob>& job) {
                             return job->id == job_id;
                         });
    
    if (it != batch_manager_->active_jobs.end()) {
        (*it)->status = ExportJob::Status::Cancelled;
        return true;
    }
    
    // Note: Jobs in queue can't be individually removed easily
    return false;
}

std::vector<std::string> ExportSystem::get_batch_job_ids() const {
    std::vector<std::string> ids;
    
    if (batch_manager_) {
        std::lock_guard<std::mutex> lock(batch_manager_->queue_mutex);
        
        for (const auto& job : batch_manager_->active_jobs) {
            ids.push_back(job->id);
        }
        
        for (const auto& job : batch_manager_->completed_jobs) {
            ids.push_back(job->id);
        }
    }
    
    return ids;
}

ExportJob* ExportSystem::get_batch_job(const std::string& job_id) {
    if (!batch_manager_) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(batch_manager_->queue_mutex);
    
    // Check active jobs
    auto it = std::find_if(batch_manager_->active_jobs.begin(), batch_manager_->active_jobs.end(),
                         [&job_id](const std::unique_ptr<ExportJob>& job) {
                             return job->id == job_id;
                         });
    
    if (it != batch_manager_->active_jobs.end()) {
        return it->get();
    }
    
    // Check completed jobs
    it = std::find_if(batch_manager_->completed_jobs.begin(), batch_manager_->completed_jobs.end(),
                     [&job_id](const std::unique_ptr<ExportJob>& job) {
                         return job->id == job_id;
                     });
    
    if (it != batch_manager_->completed_jobs.end()) {
        return it->get();
    }
    
    return nullptr;
}

void ExportSystem::start_batch_export() {
    if (batch_manager_) {
        batch_manager_->queue_cv.notify_all();
    }
}

void ExportSystem::pause_batch_export() {
    // Implementation would pause worker threads
    // For now, this is a placeholder
}

void ExportSystem::resume_batch_export() {
    if (batch_manager_) {
        batch_manager_->queue_cv.notify_all();
    }
}

void ExportSystem::cancel_batch_export() {
    if (!batch_manager_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(batch_manager_->queue_mutex);
    
    // Cancel all active jobs
    for (auto& job : batch_manager_->active_jobs) {
        job->status = ExportJob::Status::Cancelled;
    }
    
    // Clear queue
    while (!batch_manager_->job_queue.empty()) {
        batch_manager_->job_queue.pop();
    }
}

ExportConfig ExportSystem::get_preset(ExportPreset preset) const {
    if (!preset_manager_) {
        return ExportConfig{};
    }
    
    auto it = preset_manager_->presets.find(preset);
    if (it != preset_manager_->presets.end()) {
        return it->second;
    }
    
    return ExportConfig::from_preset(preset);
}

std::vector<ExportPreset> ExportSystem::get_available_presets() const {
    std::vector<ExportPreset> presets;
    
    if (preset_manager_) {
        for (const auto& pair : preset_manager_->presets) {
            presets.push_back(pair.first);
        }
    }
    
    return presets;
}

std::vector<ExportConfig> ExportSystem::get_custom_presets() const {
    if (!preset_manager_) {
        return {};
    }
    
    return preset_manager_->custom_presets;
}

bool ExportSystem::add_custom_preset(const std::string& name, const ExportConfig& config) {
    if (!preset_manager_) {
        return false;
    }
    
    auto custom_config = config;
    custom_config.preset = ExportPreset::Custom;
    
    preset_manager_->custom_presets.push_back(custom_config);
    preset_manager_->save_custom_presets();
    
    return true;
}

bool ExportSystem::remove_custom_preset(const std::string& name) {
    if (!preset_manager_) {
        return false;
    }
    
    // This is a simplified implementation
    // In practice, would need to find by name
    preset_manager_->save_custom_presets();
    return true;
}

bool ExportSystem::update_custom_preset(const std::string& name, const ExportConfig& config) {
    if (!preset_manager_) {
        return false;
    }
    
    // This is a simplified implementation
    // In practice, would need to find and update by name
    preset_manager_->save_custom_presets();
    return true;
}

bool ExportSystem::validate_export_config(const ExportConfig& config) const {
    auto validated = config.validate();
    return validated.is_valid();
}

bool ExportSystem::test_export_settings(const ExportConfig& config) {
    // Create a test configuration
    auto test_config = config;
    test_config.output_path = std::filesystem::temp_directory_path() / "test_export.mp4";
    
    // Run a short test export (first 100 frames or 3 seconds)
    // This is a placeholder implementation
    return true;
}

std::vector<std::string> ExportSystem::get_validation_warnings(const ExportConfig& config) const {
    std::vector<std::string> warnings;
    
    // Check for common issues
    if (config.bitrate > 50000000) { // 50 Mbps
        warnings.push_back("Very high bitrate may cause playback issues on some devices");
    }
    
    if (config.frame_rate > 60.0f && config.format == ExportFormat::MP4_H264) {
        warnings.push_back("High frame rates with H.264 may not be supported by all players");
    }
    
    if (config.width * config.height > 8294400) { // 4K
        warnings.push_back("4K and higher resolutions require significant processing power");
    }
    
    if (!config.use_hardware_acceleration && config.width * config.height > 2073600) { // 1080p
        warnings.push_back("Software encoding at high resolutions may be very slow");
    }
    
    return warnings;
}

const ExportSystem::ExportStatistics& ExportSystem::get_statistics() const {
    return stats_;
}

void ExportSystem::reset_statistics() {
    stats_ = ExportStatistics{};
}

void ExportSystem::export_statistics(const std::string& file_path) const {
    std::ofstream file(file_path);
    if (!file.is_open()) {
        return;
    }
    
    file << "Export Statistics\n";
    file << "================\n\n";
    file << "Total Exports: " << stats_.total_exports << "\n";
    file << "Successful: " << stats_.successful_exports << "\n";
    file << "Failed: " << stats_.failed_exports << "\n";
    file << "Success Rate: " << 
        (stats_.total_exports > 0 ? 
         (static_cast<float>(stats_.successful_exports) / stats_.total_exports * 100.0f) : 0.0f) 
        << "%\n\n";
    
    file << "Total Export Time: " << stats_.total_export_time.count() << "ms\n";
    file << "Average Export Time: " << stats_.average_export_time.count() << "ms\n\n";
    
    file << "Format Usage:\n";
    for (const auto& pair : stats_.format_usage) {
        file << "  " << ExportUtils::format_to_string(pair.first) << ": " << pair.second << "\n";
    }
}

void ExportSystem::import_statistics(const std::string& file_path) {
    // Placeholder implementation for importing statistics
}

void ExportSystem::set_max_concurrent_batch_jobs(size_t count) {
    if (batch_manager_) {
        batch_manager_->max_concurrent_jobs = count;
    }
}

size_t ExportSystem::get_max_concurrent_batch_jobs() const {
    return batch_manager_ ? batch_manager_->max_concurrent_jobs : 0;
}

void ExportSystem::set_export_buffer_size(size_t size_bytes) {
    // Would update buffer sizes in export session
}

size_t ExportSystem::get_export_buffer_size() const {
    return 64 * 1024 * 1024; // Default 64MB
}

// ============================================================================
// PRIVATE IMPLEMENTATION
// ============================================================================

bool ExportSystem::create_export_session(const ExportConfig& config) {
    cleanup_export_session();
    
    current_session_ = std::make_unique<ExportSession>();
    current_session_->config = config;
    
    // Initialize encoder based on hardware acceleration preference
    if (config.use_hardware_acceleration && hw_caps_.nvenc_available) {
        current_session_->nvenc_encoder = std::make_unique<NVENCEncoder>();
        // Initialize NVENC encoder
    } else {
        current_session_->encoder = std::make_unique<FFmpegEncoder>();
        // Initialize FFmpeg encoder
    }
    
    return current_session_->encoder || current_session_->nvenc_encoder;
}

void ExportSystem::cleanup_export_session() {
    if (!current_session_) {
        return;
    }
    
    if (current_session_->export_thread.joinable()) {
        current_session_->export_thread.join();
    }
    
    current_session_.reset();
}

bool ExportSystem::export_timeline(const ExportConfig& config) {
    // This is a simplified implementation
    // In practice, would integrate with timeline, effects, and audio systems
    
    auto start_time = std::chrono::steady_clock::now();
    bool success = false;
    
    try {
        if (config.use_hardware_acceleration && current_session_->nvenc_encoder) {
            success = export_with_nvenc(config);
        } else if (current_session_->encoder) {
            success = export_with_ffmpeg(config);
        }
    } catch (const std::exception& e) {
        current_session_->progress.set_error(e.what());
        success = false;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto export_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    update_statistics(success, export_time);
    return success;
}

bool ExportSystem::export_with_ffmpeg(const ExportConfig& config) {
    if (!current_session_->encoder) {
        return false;
    }
    
    // Initialize FFmpeg encoder
    if (!current_session_->encoder->initialize_with_hw_accel(config.output_path, config.hardware_device)) {
        return false;
    }
    
    // Set encoder parameters
    current_session_->encoder->set_resolution(config.width, config.height);
    current_session_->encoder->set_pixel_format(config.pixel_format);
    current_session_->encoder->set_bitrate(config.bitrate);
    
    // Get timeline data for export
    auto& timeline_manager = Shotcut::Timeline::TimelineManager::get();
    auto all_clips = timeline_manager.get_all_video_clips();
    
    if (all_clips.empty()) {
        current_session_->progress.set_error("No video clips found in timeline");
        return false;
    }
    
    // Calculate total frames from timeline duration
    float timeline_duration = timeline_manager.get_duration_seconds();
    int64_t total_frames = static_cast<int64_t>(timeline_duration * config.frame_rate);
    current_session_->frames_total = total_frames;
    
    // Main export loop
    int64_t current_frame = 0;
    
    while (current_frame < total_frames && !current_session_->should_cancel) {
        float current_time = static_cast<float>(current_frame) / config.frame_rate;
        
        // Get frame from timeline with effects applied
        auto frame_data = timeline_manager.get_composed_frame_at_time(current_time);
        
        if (frame_data) {
            // Apply any final export effects
            if (!apply_effects_to_frame(frame_data, config)) {
                current_session_->progress.set_error("Failed to apply export effects");
                return false;
            }
            
            // Convert to FFmpeg frame and encode
            AVFramePtr av_frame = convert_gpu_frame_to_avframe(frame_data);
            if (av_frame) {
                if (!current_session_->encoder->encode_frame(av_frame.get())) {
                    break;
                }
            }
            
            current_frame++;
            
            // Update progress
            if (current_frame % 10 == 0) { // Update every 10 frames
                {
                    std::lock_guard<std::mutex> lock(current_session_->progress_mutex);
                    current_session_->progress.update(current_frame, total_frames, start_time);
                }
            }
        }
    }
    
    // Process audio if included
    if (config.include_audio) {
        if (!export_audio_track(config, total_frames)) {
            current_session_->progress.set_error("Failed to export audio track");
            return false;
        }
    }
    
    // Flush encoder
    current_session_->encoder->flush_encoder();
    
    return current_frame == total_frames || current_session_->should_cancel;
}

bool ExportSystem::export_with_nvenc(const ExportConfig& config) {
    if (!current_session_->nvenc_encoder) {
        return false;
    }
    
    // Initialize NVENC encoder
    NVENCConfig nvenc_config = NVENCConfig::create_default();
    nvenc_config.width = config.width;
    nvenc_config.height = config.height;
    nvenc_config.target_bitrate = config.bitrate;
    
    if (!current_session_->nvenc_encoder->initialize(nvenc_config)) {
        return false;
    }
    
    // Main export loop (simplified)
    int64_t total_frames = 1000; // Placeholder - would get from timeline
    int64_t current_frame = 0;
    
    while (current_frame < total_frames && !current_session_->should_cancel) {
        // Get frame from timeline (placeholder)
        std::vector<uint8_t> frame_data; // Would get from timeline system
        
        if (!frame_data.empty()) {
            // Encode frame
            if (!current_session_->nvenc_encoder->encode_frame(frame_data.data(), frame_data.size())) {
                break;
            }
            
            current_frame++;
            
            // Update progress
            if (current_frame % 10 == 0) { // Update every 10 frames
                {
                    std::lock_guard<std::mutex> lock(current_session_->progress_mutex);
                    current_session_->progress.update(current_frame, total_frames, start_time);
                }
            }
        }
    }
    
    // Flush encoder
    current_session_->nvenc_encoder->flush_encoder();
    
    return current_frame == total_frames || current_session_->should_cancel;
}

void ExportSystem::update_progress(ExportSession* session) {
    if (!session) return;
    
    std::lock_guard<std::mutex> lock(session->progress_mutex);
    // Progress is updated in export functions
}

void ExportSystem::update_statistics(bool success, std::chrono::milliseconds export_time) {
    stats_.total_exports++;
    
    if (success) {
        stats_.successful_exports++;
    } else {
        stats_.failed_exports++;
    }
    
    stats_.total_export_time += export_time;
    if (stats_.total_exports > 0) {
        stats_.average_export_time = stats_.total_export_time / stats_.total_exports;
    }
}

// ============================================================================
// BATCH EXPORT MANAGER IMPLEMENTATION
// ============================================================================

void ExportSystem::BatchExportManager::worker_function() {
    while (!shutdown_requested) {
        std::unique_ptr<ExportJob> job;
        
        // Wait for job
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] { 
                return !job_queue.empty() || shutdown_requested; 
            });
            
            if (shutdown_requested) {
                break;
            }
            
            if (!job_queue.empty()) {
                job = std::move(job_queue.front());
                job_queue.pop();
            }
        }
        
        if (job) {
            process_job(std::move(job));
        }
    }
}

void ExportSystem::BatchExportManager::process_job(std::unique_ptr<ExportJob> job) {
    job->status = ExportJob::Status::Processing;
    job->start_time = std::chrono::steady_clock::now();
    
    // Add to active jobs
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        active_jobs.push_back(std::move(job));
    }
    
    // Process job (simplified - would use ExportSystem)
    auto& current_job = active_jobs.back();
    
    // Export logic here
    bool success = true; // Placeholder
    
    job->status = success ? ExportJob::Status::Completed : ExportJob::Status::Failed;
    job->end_time = std::chrono::steady_clock::now();
    
    // Move to completed jobs
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        completed_jobs.push_back(std::move(active_jobs.back()));
        active_jobs.pop_back();
        
        cleanup_completed_jobs();
    }
}

void ExportSystem::BatchExportManager::cleanup_completed_jobs() {
    // Keep only recent completed jobs
    if (completed_jobs.size() > max_completed_jobs) {
        completed_jobs.erase(completed_jobs.begin(), 
                         completed_jobs.begin() + (completed_jobs.size() - max_completed_jobs));
    }
}

// ============================================================================
// PRESET MANAGER IMPLEMENTATION
// ============================================================================

void ExportSystem::PresetManager::load_builtin_presets() {
    // Load all preset configurations
    presets[ExportPreset::YouTube_1080p] = ExportConfig::from_preset(ExportPreset::YouTube_1080p);
    presets[ExportPreset::YouTube_4K] = ExportConfig::from_preset(ExportPreset::YouTube_4K);
    presets[ExportPreset::YouTube_720p] = ExportConfig::from_preset(ExportPreset::YouTube_720p);
    presets[ExportPreset::YouTube_Shorts] = ExportConfig::from_preset(ExportPreset::YouTube_Shorts);
    
    presets[ExportPreset::Vimeo_1080p] = ExportConfig::from_preset(ExportPreset::Vimeo_1080p);
    presets[ExportPreset::Vimeo_4K] = ExportConfig::from_preset(ExportPreset::Vimeo_4K);
    presets[ExportPreset::Vimeo_720p] = ExportConfig::from_preset(ExportPreset::Vimeo_720p);
    
    presets[ExportPreset::ProRes_422] = ExportConfig::from_preset(ExportPreset::ProRes_422);
    
    presets[ExportPreset::Instagram_1080p] = ExportConfig::from_preset(ExportPreset::Instagram_1080p);
    presets[ExportPreset::TikTok_1080p] = ExportConfig::from_preset(ExportPreset::TikTok_1080p);
    
    presets[ExportPreset::Web_1080p] = ExportConfig::from_preset(ExportPreset::Web_1080p);
}

void ExportSystem::PresetManager::load_custom_presets() {
    // Load custom presets from file
    std::ifstream file("custom_presets.json");
    // Implementation would parse JSON and load custom presets
}

void ExportSystem::PresetManager::save_custom_presets() {
    // Save custom presets to file
    std::ofstream file("custom_presets.json");
    // Implementation would serialize to JSON
}

// ============================================================================
// HARDWARE CAPABILITIES IMPLEMENTATION
// ============================================================================

void ExportSystem::HardwareCapabilities::detect_capabilities() {
    // Check NVIDIA availability
    nvenc_available = NVIDIAVideoSDKManager::get().is_nvenc_available();
    nvdec_available = NVIDIAVideoSDKManager::get().is_nvdec_available();
    cuda_devices = NVIDIAVideoSDKManager::get().get_available_cuda_devices();
    
    // Check other hardware acceleration
    // This is simplified - would check for VA-API, VideoToolbox, etc.
    vaapi_available = false;
    videotoolbox_available = false;
}

// ============================================================================
// EXPORT MANAGER IMPLEMENTATION
// ============================================================================

std::unique_ptr<ExportSystem> ExportManager::instance_;
std::mutex ExportManager::instance_mutex_;

// ============================================================================
// EXPORT UTILS IMPLEMENTATION
// ============================================================================

namespace ExportUtils {

std::string format_to_string(ExportFormat format) {
    switch (format) {
        case ExportFormat::MP4_H264: return "H.264 MP4";
        case ExportFormat::MP4_H265: return "H.265 MP4";
        case ExportFormat::MP4_AV1: return "AV1 MP4";
        case ExportFormat::MKV_H264: return "H.264 MKV";
        case ExportFormat::MKV_H265: return "H.265 MKV";
        case ExportFormat::WEBM_VP9: return "VP9 WebM";
        case ExportFormat::WEBM_AV1: return "AV1 WebM";
        case ExportFormat::MOV_PRORES: return "ProRes MOV";
        case ExportFormat::MXF_DNXHD: return "DNxHD MXF";
        case ExportFormat::MPEG_TS: return "MPEG-TS";
        case ExportFormat::FLV: return "Flash Video";
        case ExportFormat::AVI: return "AVI";
        case ExportFormat::GIF: return "Animated GIF";
        default: return "Unknown";
    }
}

std::string quality_to_string(ExportQuality quality) {
    switch (quality) {
        case ExportQuality::Draft: return "Draft";
        case ExportQuality::Low: return "Low";
        case ExportQuality::Medium: return "Medium";
        case ExportQuality::High: return "High";
        case ExportQuality::Ultra: return "Ultra";
        case ExportQuality::Lossless: return "Lossless";
        case ExportQuality::Custom: return "Custom";
        default: return "Unknown";
    }
}

std::string preset_to_string(ExportPreset preset) {
    switch (preset) {
        case ExportPreset::YouTube_1080p: return "YouTube 1080p";
        case ExportPreset::YouTube_4K: return "YouTube 4K";
        case ExportPreset::YouTube_720p: return "YouTube 720p";
        case ExportPreset::YouTube_Shorts: return "YouTube Shorts";
        case ExportPreset::Vimeo_1080p: return "Vimeo 1080p";
        case ExportPreset::Vimeo_4K: return "Vimeo 4K";
        case ExportPreset::Vimeo_720p: return "Vimeo 720p";
        case ExportPreset::ProRes_422: return "ProRes 422";
        case ExportPreset::Instagram_1080p: return "Instagram 1080p";
        case ExportPreset::TikTok_1080p: return "TikTok 1080p";
        case ExportPreset::Web_1080p: return "Web 1080p";
        default: return "Custom";
    }
}

std::string get_file_extension(ExportFormat format) {
    switch (format) {
        case ExportFormat::MP4_H264:
        case ExportFormat::MP4_H265:
        case ExportFormat::MP4_AV1:
            return ".mp4";
        case ExportFormat::MKV_H264:
        case ExportFormat::MKV_H265:
            return ".mkv";
        case ExportFormat::WEBM_VP9:
        case ExportFormat::WEBM_AV1:
            return ".webm";
        case ExportFormat::MOV_PRORES:
            return ".mov";
        case ExportFormat::MXF_DNXHD:
            return ".mxf";
        case ExportFormat::MPEG_TS:
            return ".ts";
        case ExportFormat::FLV:
            return ".flv";
        case ExportFormat::AVI:
            return ".avi";
        case ExportFormat::GIF:
            return ".gif";
        default:
            return ".mp4";
    }
}

std::string generate_output_filename(const std::string& input_path, 
                                ExportFormat format, 
                                const std::string& suffix) {
    std::filesystem::path input(input_path);
    std::string filename = input.stem().string();
    
    if (!suffix.empty()) {
        filename += "_" + suffix;
    }
    
    filename += get_file_extension(format);
    return filename;
}

bool ensure_output_directory(const std::string& file_path) {
    std::filesystem::path path(file_path);
    std::filesystem::path dir = path.parent_path();
    
    return std::filesystem::create_directories(dir);
}

uint64_t get_available_disk_space(const std::string& path) {
    std::filesystem::space_info space = std::filesystem::space(path);
    return space.available;
}

int64_t calculate_bitrate(int width, int height, float fps, ExportQuality quality) {
    // Base bitrate calculation
    double pixels_per_second = width * height * fps;
    double base_bitrate = pixels_per_second * 0.1; // 0.1 bits per pixel per second
    
    // Quality multiplier
    double quality_multiplier = 1.0;
    switch (quality) {
        case ExportQuality::Draft: quality_multiplier = 0.3; break;
        case ExportQuality::Low: quality_multiplier = 0.6; break;
        case ExportQuality::Medium: quality_multiplier = 1.0; break;
        case ExportQuality::High: quality_multiplier = 1.5; break;
        case ExportQuality::Ultra: quality_multiplier = 2.5; break;
        case ExportQuality::Lossless: quality_multiplier = 5.0; break;
        default: break;
    }
    
    return static_cast<int64_t>(base_bitrate * quality_multiplier);
}

int calculate_optimal_gop_size(float fps) {
    // GOP size should be roughly 1-2 seconds of video
    return static_cast<int>(fps * 1.5);
}

int calculate_optimal_buffer_size(int width, int height, ExportFormat format) {
    // Calculate buffer size based on frame size and format
    int64_t frame_size = width * height * 4; // Assume 4 bytes per pixel
    return static_cast<int>(frame_size * 16); // 16 frames buffer
}

std::chrono::milliseconds estimate_export_time(
    int64_t total_frames, 
    float fps, 
    ExportFormat format,
    bool hardware_acceleration
) {
    // Estimate based on format and hardware acceleration
    float processing_speed = hardware_acceleration ? 120.0f : 30.0f; // fps
    
    auto duration_ms = static_cast<int64_t>((total_frames / fps) * 1000.0f / processing_speed * fps);
    
    return std::chrono::milliseconds(duration_ms);
}

size_t estimate_output_size(
    int64_t duration_ms,
    int64_t bitrate,
    bool include_audio,
    int64_t audio_bitrate
) {
    int64_t total_bitrate = bitrate;
    if (include_audio) {
        total_bitrate += audio_bitrate;
    }
    
    return static_cast<size_t>((total_bitrate * duration_ms) / (8 * 1000)); // Convert to bytes
}

} // namespace ExportUtils

// ============================================================================
// MISSING HELPER FUNCTIONS IMPLEMENTATION
// ============================================================================

bool ExportSystem::apply_effects_to_frame(std::shared_ptr<GPUFrame> frame, const ExportConfig& config) {
    if (!frame) {
        return false;
    }
    
    // Get effects system and apply export effects
    auto& effects_manager = Shotcut::Effects::EffectSystem::get();
    
    // Apply final export effects (color correction, etc.)
    try {
        effects_manager.apply_effects_to_frame(frame, config.export_effects);
    } catch (const std::exception& e) {
        current_session_->progress.set_error("Effects application failed: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

bool ExportSystem::export_audio_track(const ExportConfig& config, int64_t total_frames) {
    if (!config.include_audio) {
        return true;
    }
    
    // Get audio processor
    auto& audio_processor = Shotcut::Audio::AudioProcessor::get();
    
    try {
        // Initialize audio export
        if (!audio_processor.initialize_export(config.audio_sample_rate, config.audio_channels)) {
            current_session_->progress.set_error("Failed to initialize audio export");
            return false;
        }
        
        // Process audio timeline
        auto audio_clips = Shotcut::Timeline::TimelineManager::get().get_all_audio_clips();
        
        for (const auto& clip : audio_clips) {
            if (!audio_processor.process_clip_for_export(clip, config)) {
                current_session_->progress.set_error("Failed to process audio clip: " + clip.name);
                return false;
            }
        }
        
        // Export audio data
        if (!audio_processor.export_audio_file(config.output_path)) {
            current_session_->progress.set_error("Failed to export audio file");
            return false;
        }
        
    } catch (const std::exception& e) {
        current_session_->progress.set_error("Audio export failed: " + std::string(e.what()));
        return false;
    }
    
    return true;
}

AVFramePtr ExportSystem::convert_gpu_frame_to_avframe(std::shared_ptr<GPUFrame> frame) {
    if (!frame) {
        return nullptr;
    }
    
    // Create FFmpeg frame from GPU frame
    AVFramePtr av_frame = std::make_unique<AVFrame>();
    if (!av_frame) {
        return nullptr;
    }
    
    // Allocate frame buffer
    av_frame->format = AV_PIX_FMT_YUV420P;
    av_frame->width = frame->width;
    av_frame->height = frame->height;
    
    int ret = av_frame_get_buffer(av_frame.get(), 0);
    if (ret < 0) {
        return nullptr;
    }
    
    // Copy data from GPU frame to CPU frame
    // This is simplified - in practice would use GPU-to-CPU memory copy
    auto frame_data = frame->get_cpu_data();
    if (frame_data.empty()) {
        return nullptr;
    }
    
    // Copy frame data (simplified)
    std::memcpy(av_frame->data[0], frame_data.data(), frame_data.size());
    
    av_frame->pts = frame->timestamp;
    av_frame->key_frame = (frame->frame_type == FrameType::I_FRAME);
    
    return av_frame;
}

} // namespace Shotcut::Video
