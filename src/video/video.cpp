// Video processing module for ShotcutCPP
// Professional-grade video processing with hardware acceleration

#include "video/ffmpeg_wrapper.h"
#include "video/frame_buffer.h"
#include "video/video_preview.h"
#include <iostream>
#include <memory>

namespace Shotcut::Video {

// Global video system state
static std::unique_ptr<CodecManager> g_codec_manager;
static std::unique_ptr<FramePool> g_frame_pool;
static std::unique_ptr<PreviewManager> g_preview_manager;
static VkDevice g_vulkan_device = VK_NULL_HANDLE;
static bool g_video_initialized = false;

// Video system initialization
bool initialize() {
    if (g_video_initialized) {
        std::cout << "Video system already initialized" << std::endl;
        return true;
    }
    
    try {
        std::cout << "Initializing Shotcut Video Processing System..." << std::endl;
        
        // Initialize codec manager
        g_codec_manager = std::make_unique<CodecManager>();
        if (!g_codec_manager->initialize()) {
            std::cerr << "Failed to initialize codec manager" << std::endl;
            return false;
        }
        
        std::cout << "✓ Codec Manager initialized" << std::endl;
        
        // Initialize frame pool (will use placeholder Vulkan device for now)
        g_frame_pool = std::make_unique<FramePool>(g_vulkan_device, 2ULL * 1024 * 1024 * 1024);
        
        // Preload common frame formats
        g_frame_pool->preload_common_formats();
        
        std::cout << "✓ Frame Pool initialized" << std::endl;
        
        // Initialize preview manager
        g_preview_manager = std::make_unique<PreviewManager>(g_vulkan_device);
        
        std::cout << "✓ Preview Manager initialized" << std::endl;
        
        g_video_initialized = true;
        
        // Display system capabilities
        display_system_info();
        
        std::cout << "🎬 Video Processing System initialized successfully!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Video system initialization failed: " << e.what() << std::endl;
        shutdown();
        return false;
    }
}

// Video system cleanup
void shutdown() {
    if (!g_video_initialized) {
        return;
    }
    
    std::cout << "Shutting down Shotcut Video Processing System..." << std::endl;
    
    // Cleanup in reverse order
    if (g_preview_manager) {
        g_preview_manager.reset();
        std::cout << "✓ Preview Manager shutdown" << std::endl;
    }
    
    if (g_frame_pool) {
        g_frame_pool.reset();
        std::cout << "✓ Frame Pool shutdown" << std::endl;
    }
    
    if (g_codec_manager) {
        g_codec_manager->cleanup();
        g_codec_manager.reset();
        std::cout << "✓ Codec Manager shutdown" << std::endl;
    }
    
    g_video_initialized = false;
    
    std::cout << "🎬 Video Processing System shutdown complete" << std::endl;
}

// System information
void display_system_info() {
    if (!g_codec_manager) {
        return;
    }
    
    std::cout << "\n=== Video System Information ===" << std::endl;
    
    // Display available codecs
    const auto& codecs = g_codec_manager->get_available_codecs();
    std::cout << "Available Codecs: " << codecs.size() << std::endl;
    
    int decoder_count = 0;
    int encoder_count = 0;
    for (const auto& codec : codecs) {
        if (codec.can_decode) decoder_count++;
        if (codec.can_encode) encoder_count++;
    }
    std::cout << "  Decoders: " << decoder_count << std::endl;
    std::cout << "  Encoders: " << encoder_count << std::endl;
    
    // Display hardware acceleration
    const auto& hw_devices = g_codec_manager->get_available_hw_devices();
    std::cout << "Hardware Acceleration: ";
    if (hw_devices.empty()) {
        std::cout << "None available" << std::endl;
    } else {
        for (size_t i = 0; i < hw_devices.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << hw_devices[i];
        }
        std::cout << std::endl;
    }
    
    // Display frame pool statistics
    if (g_frame_pool) {
        std::cout << "Frame Pool Memory: " 
                  << (g_frame_pool->get_total_memory_used() / (1024 * 1024)) << "MB used" << std::endl;
        std::cout << "Memory Usage: " << g_frame_pool->get_memory_usage_percentage() << "%" << std::endl;
    }
    
    std::cout << "================================" << std::endl;
}

// Codec manager access
CodecManager& get_codec_manager() {
    if (!g_codec_manager) {
        throw std::runtime_error("Codec manager not initialized");
    }
    return *g_codec_manager;
}

// Frame pool access
FramePool& get_frame_pool() {
    if (!g_frame_pool) {
        throw std::runtime_error("Frame pool not initialized");
    }
    return *g_frame_pool;
}

// Preview manager access
PreviewManager& get_preview_manager() {
    if (!g_preview_manager) {
        throw std::runtime_error("Preview manager not initialized");
    }
    return *g_preview_manager;
}

// Utility functions
bool is_initialized() {
    return g_video_initialized;
}

// Video file analysis
struct VideoFileInfo {
    std::string file_path;
    int width = 0;
    int height = 0;
    float fps = 0.0f;
    int64_t duration_us = 0;
    std::string codec_name;
    int64_t bitrate = 0;
    bool has_audio = false;
};

std::optional<VideoFileInfo> analyze_video_file(const std::string& file_path) {
    try {
        FFmpegDecoder decoder;
        if (!decoder.initialize(file_path)) {
            return std::nullopt;
        }
        
        VideoFileInfo info;
        info.file_path = file_path;
        info.width = decoder.get_width();
        info.height = decoder.get_height();
        info.duration_us = decoder.get_duration();
        info.bitrate = decoder.get_bitrate();
        
        // Calculate FPS
        AVRational frame_rate = decoder.get_frame_rate();
        if (frame_rate.den > 0) {
            info.fps = static_cast<float>(frame_rate.num) / static_cast<float>(frame_rate.den);
        }
        
        // Get codec info
        auto codec_info = decoder.get_codec_info();
        if (codec_info) {
            info.codec_name = codec_info->name;
        }
        
        // Check for audio (simplified)
        info.has_audio = false; // Would need stream analysis
        
        return info;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to analyze video file: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// Demo function to showcase video processing capabilities
void demo_video_processing() {
    std::cout << "\n=== Video Processing Demo ===" << std::endl;
    
    if (!is_initialized()) {
        std::cout << "Video system not initialized!" << std::endl;
        return;
    }
    
    // Demo codec capabilities
    std::cout << "\n1. Testing Codec Manager..." << std::endl;
    auto& codec_manager = get_codec_manager();
    
    // Test H.264 codec
    auto h264_info = codec_manager.get_codec_info("h264");
    if (h264_info) {
        std::cout << "  ✓ H.264 codec available: " << (*h264_info)->long_name << std::endl;
        std::cout << "    Can decode: " << ((*h264_info)->can_decode ? "Yes" : "No") << std::endl;
        std::cout << "    Can encode: " << ((*h264_info)->can_encode ? "Yes" : "No") << std::endl;
    }
    
    // Demo frame pool
    std::cout << "\n2. Testing Frame Pool..." << std::endl;
    auto& frame_pool = get_frame_pool();
    
    FrameDescriptor test_desc = {1920, 1080, PixelFormat::RGBA, 0, 0};
    auto frame = frame_pool.allocate_frame(test_desc);
    if (frame) {
        std::cout << "  ✓ Successfully allocated 1920x1080 RGBA frame" << std::endl;
        frame_pool.return_frame(frame);
    }
    
    // Display frame pool statistics
    auto stats = frame_pool.get_all_statistics();
    if (!stats.empty()) {
        std::cout << "  Frame pool statistics:" << std::endl;
        for (const auto& stat : stats) {
            std::cout << "    Allocated: " << stat.total_allocated 
                      << ", Cache hits: " << stat.cache_hits 
                      << ", Hit rate: " << (stat.hit_rate * 100.0) << "%" << std::endl;
        }
    }
    
    std::cout << "\n=== Demo Complete ===" << std::endl;
}

} // namespace Video
} // namespace Shotcut
