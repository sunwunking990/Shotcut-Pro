// Video Processing Demo for ShotcutCPP
// Demonstrates the complete video processing pipeline

#include "video/video.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace Shotcut::Video;

void demonstrate_codec_system() {
    std::cout << "\n=== Codec System Demo ===" << std::endl;
    
    // Test codec manager
    auto& codec_manager = Video::get_codec_manager();
    
    // List available codecs
    const auto& codecs = codec_manager.get_available_codecs();
    std::cout << "Available codecs: " << codecs.size() << std::endl;
    
    // Test specific codecs
    auto test_codec = [&](const std::string& name) {
        auto info = codec_manager.get_codec_info(name);
        if (info) {
            std::cout << "✓ " << name << ": " << (*info)->long_name << std::endl;
            std::cout << "  Decode: " << ((*info)->can_decode ? "Yes" : "No") << std::endl;
            std::cout << "  Encode: " << ((*info)->can_encode ? "Yes" : "No") << std::endl;
            std::cout << "  HW Accelerated: " << ((*info)->hardware_accelerated ? "Yes" : "No") << std::endl;
        } else {
            std::cout << "✗ " << name << ": Not available" << std::endl;
        }
    };
    
    test_codec("h264");
    test_codec("hevc");
    test_codec("vp9");
    test_codec("av1");
    
    // Test hardware acceleration
    const auto& hw_devices = codec_manager.get_available_hw_devices();
    std::cout << "\nHardware acceleration devices: " << hw_devices.size() << std::endl;
    for (const auto& device : hw_devices) {
        std::cout << "  " << device << std::endl;
    }
}

void demonstrate_frame_buffer_system() {
    std::cout << "\n=== Frame Buffer System Demo ===" << std::endl;
    
    auto& frame_pool = Video::get_frame_pool();
    
    // Test different frame formats
    std::vector<FrameDescriptor> test_formats = {
        {1920, 1080, PixelFormat::RGBA, 0, 0},   // Full HD
        {1280, 720, PixelFormat::RGBA, 0, 0},    // HD
        {3840, 2160, PixelFormat::RGBA, 0, 0},   // 4K
        {1920, 1080, PixelFormat::YUV420P, 0, 0}, // Video format
    };
    
    std::vector<std::shared_ptr<GPUFrame>> allocated_frames;
    
    for (const auto& desc : test_formats) {
        std::cout << "Testing " << desc.width << "x" << desc.height << " ";
        
        // Allocate frame
        auto frame = frame_pool.allocate_frame(desc);
        if (frame) {
            std::cout << "✓ Allocated" << std::endl;
            allocated_frames.push_back(frame);
            
            // Test frame properties
            std::cout << "  Valid: " << (frame->is_valid() ? "Yes" : "No") << std::endl;
            std::cout << "  Ref count: " << frame->ref_count() << std::endl;
        } else {
            std::cout << "✗ Failed to allocate" << std::endl;
        }
    }
    
    // Test frame cache
    FrameCache cache(10);
    
    // Add frames to cache
    for (size_t i = 0; i < allocated_frames.size() && i < 5; ++i) {
        std::string key = "test_frame_" + std::to_string(i);
        cache.put(key, allocated_frames[i]);
        std::cout << "Cached frame: " << key << std::endl;
    }
    
    // Test cache retrieval
    for (size_t i = 0; i < 3; ++i) {
        std::string key = "test_frame_" + std::to_string(i);
        auto cached_frame = cache.get(key);
        if (cached_frame) {
            std::cout << "Retrieved from cache: " << key << std::endl;
        }
    }
    
    // Display cache statistics
    std::cout << "Cache hit rate: " << (cache.get_hit_rate() * 100.0) << "%" << std::endl;
    
    // Return frames to pool
    for (auto& frame : allocated_frames) {
        frame_pool.return_frame(frame);
    }
    
    // Display pool statistics
    auto stats = frame_pool.get_all_statistics();
    std::cout << "Frame pool statistics:" << std::endl;
    for (const auto& stat : stats) {
        std::cout << "  Allocated: " << stat.total_allocated 
                  << ", Cache hits: " << stat.cache_hits 
                  << ", Hit rate: " << (stat.hit_rate * 100.0) << "%" << std::endl;
    }
}

void demonstrate_video_decoding() {
    std::cout << "\n=== Video Decoding Demo ===" << std::endl;
    
    FFmpegDecoder decoder;
    
    // Test with placeholder file (will fail gracefully)
    std::string test_file = "test_video.mp4";
    
    std::cout << "Testing decoder initialization..." << std::endl;
    if (decoder.initialize(test_file)) {
        std::cout << "✓ Decoder initialized for: " << test_file << std::endl;
        
        // Get video information
        std::cout << "Video information:" << std::endl;
        std::cout << "  Resolution: " << decoder.get_width() << "x" << decoder.get_height() << std::endl;
        std::cout << "  Duration: " << (decoder.get_duration() / 1000000.0) << " seconds" << std::endl;
        std::cout << "  Bitrate: " << (decoder.get_bitrate() / 1000000.0) << " Mbps" << std::endl;
        
        // Test frame decoding (simulated)
        std::cout << "Testing frame decoding..." << std::endl;
        for (int i = 0; i < 5; ++i) {
            auto frame = decoder.decode_next_frame();
            if (frame) {
                std::cout << "  ✓ Decoded frame " << (i + 1) << std::endl;
            } else {
                std::cout << "  - Frame " << (i + 1) << " (end of file or error)" << std::endl;
                break;
            }
        }
        
        // Display decoder performance
        std::cout << "Decoder performance:" << std::endl;
        std::cout << "  Frames decoded: " << decoder.get_frames_decoded() << std::endl;
        std::cout << "  Average decode time: " << decoder.get_average_decode_time() << " ms" << std::endl;
        
        decoder.cleanup();
        
    } else {
        std::cout << "✗ Failed to initialize decoder (expected for demo)" << std::endl;
    }
}

void demonstrate_video_preview() {
    std::cout << "\n=== Video Preview Demo ===" << std::endl;
    
    // Create preview system (using placeholder Vulkan device)
    VideoPreview preview;
    
    std::cout << "Testing preview initialization..." << std::endl;
    if (preview.initialize(VK_NULL_HANDLE, VK_NULL_HANDLE, 0)) {
        std::cout << "✓ Preview system initialized" << std::endl;
        
        // Test preview configuration
        preview.set_resolution(1920, 1080);
        preview.set_quality(PreviewQuality::High);
        preview.set_target_fps(60.0f);
        
        std::cout << "Preview configuration set to 1920x1080@60fps, High quality" << std::endl;
        
        // Test zoom and pan
        preview.set_zoom(1.5f);
        preview.set_pan(0.1f, 0.1f);
        std::cout << "Zoom set to 1.5x, pan offset (0.1, 0.1)" << std::endl;
        
        preview.cleanup();
        
    } else {
        std::cout << "✗ Failed to initialize preview (expected for demo)" << std::endl;
    }
}

void demonstrate_performance_monitoring() {
    std::cout << "\n=== Performance Monitoring Demo ===" << std::endl;
    
    auto& frame_pool = Video::get_frame_pool();
    
    // Simulate intensive frame allocation
    std::cout << "Simulating intensive frame operations..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::shared_ptr<GPUFrame>> frames;
    FrameDescriptor desc = {1920, 1080, PixelFormat::RGBA, 0, 0};
    
    // Allocate and release frames rapidly
    for (int i = 0; i < 20; ++i) {
        auto frame = frame_pool.allocate_frame(desc);
        if (frame) {
            frames.push_back(frame);
        }
        
        // Release some frames
        if (i % 3 == 0 && !frames.empty()) {
            frame_pool.return_frame(frames.front());
            frames.erase(frames.begin());
        }
        
        // Small delay to simulate processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Return remaining frames
    for (auto& frame : frames) {
        frame_pool.return_frame(frame);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Completed in " << duration.count() << " ms" << std::endl;
    
    // Display final statistics
    auto stats = frame_pool.get_all_statistics();
    if (!stats.empty()) {
        const auto& stat = stats[0];
        std::cout << "Final statistics:" << std::endl;
        std::cout << "  Total allocated: " << stat.total_allocated << std::endl;
        std::cout << "  Cache hits: " << stat.cache_hits << std::endl;
        std::cout << "  Cache misses: " << stat.cache_misses << std::endl;
        std::cout << "  Hit rate: " << (stat.hit_rate * 100.0) << "%" << std::endl;
        std::cout << "  Memory usage: " << frame_pool.get_memory_usage_percentage() << "%" << std::endl;
    }
}

int main() {
    std::cout << "🎬 Shotcut Video Processing System Demo" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Initialize video system
    if (!Video::initialize()) {
        std::cerr << "Failed to initialize video system!" << std::endl;
        return 1;
    }
    
    try {
        // Run demonstrations
        demonstrate_codec_system();
        demonstrate_frame_buffer_system();
        demonstrate_video_decoding();
        demonstrate_video_preview();
        demonstrate_performance_monitoring();
        
        // Run the built-in demo
        Video::demo_video_processing();
        
        std::cout << "\n✅ All video processing demos completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demo failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    // Cleanup
    Video::shutdown();
    
    return 0;
}
