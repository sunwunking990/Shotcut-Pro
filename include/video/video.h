#pragma once

// Main video processing system header for ShotcutCPP
// Provides unified access to all video processing components

#include "video/ffmpeg_wrapper.h"
#include "video/frame_buffer.h"
#include "video/video_preview.h"
#include <optional>
#include <string>

namespace Shotcut::Video {

// Forward declarations for internal classes
class CodecManager;
class FramePool;
class PreviewManager;

// Video system initialization
bool initialize();
void shutdown();
bool is_initialized();

// System information
void display_system_info();

// Component access
CodecManager& get_codec_manager();
FramePool& get_frame_pool();
PreviewManager& get_preview_manager();

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

std::optional<VideoFileInfo> analyze_video_file(const std::string& file_path);

// Demo function
void demo_video_processing();

} // namespace Shotcut::Video
