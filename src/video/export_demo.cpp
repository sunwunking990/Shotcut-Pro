// Export System Demo
// Demonstrates professional video export capabilities with hardware acceleration

#include "video/export_system.h"
#include "video/secure_file_format.h"
#include "../include/timeline/entity.h"
#include "../include/effects/effect_system.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace Shotcut::Video;

class ExportDemo {
private:
    std::unique_ptr<ExportSystem> export_system_;
    
public:
    ExportDemo() {
        export_system_ = std::make_unique<ExportSystem>();
        export_system_->initialize();
    }
    
    ~ExportDemo() {
        if (export_system_) {
            export_system_->cleanup();
        }
    }
    
    void run_demo() {
        std::cout << "=== Shotcut C++ Export System Demo ===\n\n";
        
        // Demo 1: Hardware capability detection
        demo_hardware_capabilities();
        
        // Demo 2: Export presets showcase
        demo_export_presets();
        
        // Demo 3: Single export with progress tracking
        demo_single_export();
        
        // Demo 4: Batch export functionality
        demo_batch_export();
        
        // Demo 5: Secure file format demonstration
        demo_secure_file_format();
        
        // Demo 6: Export statistics and validation
        demo_export_statistics();
        
        std::cout << "\n=== Export System Demo Complete ===\n";
    }
    
private:
    void demo_hardware_capabilities() {
        std::cout << "1. Hardware Capabilities Detection\n";
        std::cout << "===================================\n";
        
        std::cout << "NVENC Available: " << (export_system_->is_nvenc_available() ? "Yes" : "No") << "\n";
        std::cout << "NVDEC Available: " << (export_system_->is_nvdec_available() ? "Yes" : "No") << "\n";
        std::cout << "VA-API Available: " << (export_system_->is_vaapi_available() ? "Yes" : "No") << "\n";
        
        auto cuda_devices = export_system_->get_cuda_devices();
        std::cout << "CUDA Devicesices: " << cuda_devices.size() << "\n";
        for (size_t i = 0; i < cuda_devices.size(); ++i) {
            std::cout << "  Device " << i << ": CUDA " << cuda_devices[i] << "\n";
        }
        std::cout << "\n";
    }
    
    void demo_export_presets() {
        std::cout << "2. Export Presets Showcase\n";
        std::cout << "===========================\n";
        
        auto presets = export_system_->get_available_presets();
        std::cout << "Available presets (" << presets.size() << "):\n";
        
        for (const auto& preset : presets) {
            auto config = export_system_->get_preset(preset);
            std::cout << "  " << ExportUtils::preset_to_string(preset) << ":\n";
            std::cout << "    Format: " << ExportUtils::format_to_string(config.format) << "\n";
            std::cout << "    Resolution: " << config.width << "x" << config.height << "\n";
            std::cout << "    Frame Rate: " << config.frame_rate << " fps\n";
            std::cout << "    Bitrate: " << (config.bitrate / 1000000.0) << " Mbps\n";
            std::cout << "    Hardware Acceleration: " << (config.use_hardware_acceleration ? "Enabled" : "Disabled") << "\n\n";
        }
    }
    
    void demo_single_export() {
        std::cout << "3. Single Export with Progress Tracking\n";
        std::cout << "=======================================\n";
        
        // Create export configuration
        auto config = ExportConfig::from_preset(ExportPreset::YouTube_1080p);
        config.output_path = "demo_export_1080p.mp4";
        config.overwrite_existing = true;
        config.create_thumbnail = true;
        config.validate_output = true;
        
        std::cout << "Starting export with configuration:\n";
        std::cout << "  Format: " << ExportUtils::format_to_string(config.format) << "\n";
        std::cout << "  Resolution: " << config.width << "x" << config.height << "\n";
        std::cout << "  Bitrate: " << (config.bitrate / 1000000.0) << " Mbps\n";
        std::cout << "  Hardware Acceleration: " << (config.use_hardware_acceleration ? "Enabled" : "Disabled") << "\n\n";
        
        // Set up progress tracking
        bool export_complete = false;
        bool export_success = false;
        
        auto progress_callback = [](const ExportProgress& progress) {
            std::cout << "\rProgress: " << std::fixed << std::setprecision(1) 
                     << progress.percentage << "% | "
                     << "Frames: " << progress.frames_processed << "/" << progress.total_frames << " | "
                     << "FPS: " << std::setprecision(1) << progress.fps << " | "
                     << "Time: " << progress.elapsed_time.count() / 1000.0 << "s | "
                     << "ETA: " << progress.estimated_time_remaining.count() / 1000.0 << "s";
            std::cout.flush();
        };
        
        auto completion_callback = [&export_complete, &export_success](bool success) {
            export_complete = true;
            export_success = success;
            std::cout << "\nExport " << (success ? "completed successfully" : "failed") << "!\n";
        };
        
        // Start export
        if (export_system_->start_export(config, progress_callback, completion_callback)) {
            std::cout << "Export started...\n";
            
            // Wait for completion (in real app, this would be async)
            while (!export_complete) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            if (export_success) {
                // Show export statistics
                auto progress = export_system_->get_progress();
                std::cout << "Export Statistics:\n";
                std::cout << "  Total Frames: " << progress.frames_processed << "\n";
                std::cout << "  Average FPS: " << std::fixed << std::setprecision(1) << progress.fps << "\n";
                std::cout << "  Total Time: " << progress.elapsed_time.count() / 1000.0 << "s\n";
                std::cout << "  Output Size: " << ExportUtils::estimate_output_size(
                    progress.elapsed_time.count(), 
                    config.bitrate, 
                    config.include_audio, 
                    config.audio_bitrate) / (1024 * 1024) << " MB\n";
            }
        } else {
            std::cout << "Failed to start export!\n";
        }
        
        std::cout << "\n";
    }
    
    void demo_batch_export() {
        std::cout << "4. Batch Export Functionality\n";
        std::cout << "==============================\n";
        
        // Create multiple export jobs
        std::vector<std::string> job_ids;
        
        // Job 1: YouTube 1080p
        ExportJob job1;
        job1.input_timeline_path = "demo_timeline.sct";
        job1.output_path = "batch_youtube_1080p.mp4";
        job1.config = ExportConfig::from_preset(ExportPreset::YouTube_1080p);
        job1.priority = 1;
        
        auto job1_id = export_system_->add_batch_job(job1);
        if (!job1_id.empty()) {
            job_ids.push_back(job1_id);
            std::cout << "Added YouTube 1080p job: " << job1_id << "\n";
        }
        
        // Job 2: Vimeo 4K
        ExportJob job2;
        job2.input_timeline_path = "demo_timeline.sct";
        job2.output_path = "batch_vimeo_4k.mp4";
        job2.config = ExportConfig::from_preset(ExportPreset::Vimeo_4K);
        job2.priority = 2;
        
        auto job2_id = export_system_->add_batch_job(job2);
        if (!job2_id.empty()) {
            job_ids.push_back(job2_id);
            std::cout << "Added Vimeo 4K job: " << job2_id << "\n";
        }
        
        // Job 3: Instagram Square
        ExportJob job3;
        job3.input_timeline_path = "demo_timeline.sct";
        job3.output_path = "batch_instagram_square.mp4";
        job3.config = ExportConfig::from_preset(ExportPreset::Instagram_1080p);
        job3.priority = 3;
        
        auto job3_id = export_system_->add_batch_job(job3);
        if (!job3_id.empty()) {
            job_ids.push_back(job3_id);
            std::cout << "Added Instagram Square job: " << job3_id << "\n";
        }
        
        std::cout << "\nStarting batch export...\n";
        export_system_->start_batch_export();
        
        // Monitor batch progress (simplified)
        for (int i = 0; i < 50; ++i) { // Simulate monitoring
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::cout << "\rBatch Progress: Checking jobs...";
            std::cout.flush();
            
            // In real implementation, would check individual job progress
        }
        
        std::cout << "\nBatch export completed!\n";
        std::cout << "Processed " << job_ids.size() << " jobs\n\n";
    }
    
    void demo_secure_file_format() {
        std::cout << "5. Secure File Format Demonstration\n";
        std::cout << "===================================\n";
        
        // Demo secure file creation
        std::string secure_file = "demo_secure_video.scvf";
        
        // Create secure file writer
        auto writer = std::make_unique<SecureFileWriter>(secure_file);
        
        // Configure encryption and compression
        writer->set_encryption_from_password("demo_password_123");
        writer->set_compression(CompressionAlgorithm::LZ4);
        writer->set_video_properties(1920, 1080, 30.0f);
        writer->set_audio_properties(2, 48000);
        
        std::cout << "Creating secure video file...\n";
        std::cout << "  Encryption: AES-256 GCM\n";
        std::cout << "  Compression: LZ4\n";
        std::cout << "  Video: 1920x1080 @ 30fps\n";
        std::cout << "  Audio: Stereo @ 48kHz\n\n";
        
        // Simulate writing frames
        if (writer->open()) {
            // Write header
            writer->write_header();
            
            // Simulate writing 100 frames
            for (uint32_t i = 0; i < 100; ++i) {
                std::vector<uint8_t> frame_data(1024 * 768 * 3, i % 256); // Dummy frame data
                writer->write_video_frame(frame_data, i, i * 33); // 30fps = 33ms per frame
            }
            
            writer->close();
            std::cout << "Secure file created successfully!\n";
            
            // Demo file reading
            demo_secure_file_reading(secure_file);
        } else {
            std::cout << "Failed to create secure file: " << writer->get_last_error() << "\n";
        }
        
        std::cout << "\n";
    }
    
    void demo_secure_file_reading(const std::string& file_path) {
        std::cout << "\nReading secure file...\n";
        
        auto reader = std::make_unique<SecureFileReader>(file_path);
        
        if (reader->open()) {
            // Check file integrity
            if (reader->verify_file_integrity()) {
                std::cout << "File integrity verified!\n";
            } else {
                std::cout << "File integrity check failed!\n";
                return;
            }
            
            // Display file information
            auto header = reader->get_header();
            std::cout << "File Information:\n";
            std::cout << "  Format: ShotCut Video Format v" 
                     << ((header.version >> 8) & 0xFF) << "." 
                     << (header.version & 0xFF) << "\n";
            std::cout << "  Resolution: " << header.width << "x" << header.height << "\n";
            std::cout << "  Frame Rate: " << header.frame_rate << " fps\n";
            std::cout << "  Frame Count: " << header.frame_count << "\n";
            std::cout << "  Encrypted: " << (header.flags & 0x0001 ? "Yes" : "No") << "\n";
            std::cout << "  Compressed: " << (header.flags & 0x0002 ? "Yes" : "No") << "\n";
            
            // Unlock with password
            if (reader->unlock_with_password("demo_password_123")) {
                std::cout << "File unlocked successfully!\n";
                
                // Read some frames
                auto frame_data = reader->read_video_frame(0);
                std::cout << "Read frame 0: " << frame_data.size() << " bytes\n";
                
                auto frame_data_50 = reader->read_video_frame(50);
                std::cout << "Read frame 50: " << frame_data_50.size() << " bytes\n";
                
            } else {
                std::cout << "Failed to unlock file: " << reader->get_last_error() << "\n";
            }
            
            reader->close();
        } else {
            std::cout << "Failed to open secure file: " << reader->get_last_error() << "\n";
        }
    }
    
    void demo_export_statistics() {
        std::cout << "6. Export Statistics and Validation\n";
        std::cout << "====================================\n";
        
        // Get current statistics
        auto stats = export_system_->get_statistics();
        
        std::cout << "Export System Statistics:\n";
        std::cout << "  Total Exports: " << stats.total_exports << "\n";
        std::cout << "  Successful: " << stats.successful_exports << "\n";
        std::cout << "  Failed: " << stats.failed_exports << "\n";
        std::cout << "  Success Rate: " << std::fixed << std::setprecision(1) 
                 << (stats.total_exports > 0 ? 
                    (static_cast<float>(stats.successful_exports) / stats.total_exports * 100.0f) : 0.0f) 
                 << "%\n";
        std::cout << "  Average Export Time: " << stats.average_export_time.count() << "ms\n";
        
        std::cout << "\nFormat Usage:\n";
        for (const auto& pair : stats.format_usage) {
            std::cout << "  " << ExportUtils::format_to_string(pair.first) << ": " << pair.second << "\n";
        }
        
        std::cout << "\nPreset Usage:\n";
        for (const auto& pair : stats.preset_usage) {
            std::cout << "  " << ExportUtils::preset_to_string(pair.first) << ": " << pair.second << "\n";
        }
        
        // Demo validation
        std::cout << "\nExport Configuration Validation:\n";
        
        auto test_config = ExportConfig::from_preset(ExportPreset::YouTube_4K);
        auto warnings = export_system_->get_validation_warnings(test_config);
        
        if (export_system_->validate_export_config(test_config)) {
            std::cout << "  ✓ Configuration is valid\n";
        } else {
            std::cout << "  ✗ Configuration is invalid\n";
        }
        
        if (!warnings.empty()) {
            std::cout << "  Warnings:\n";
            for (const auto& warning : warnings) {
                std::cout << "    - " << warning << "\n";
            }
        } else {
            std::cout << "  No warnings\n";
        }
        
        // Export statistics to file
        export_system_->export_statistics("export_stats.txt");
        std::cout << "\nStatistics exported to: export_stats.txt\n";
    }
};

int main() {
    try {
        ExportDemo demo;
        demo.run_demo();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Demo error: " << e.what() << std::endl;
        return 1;
    }
}
