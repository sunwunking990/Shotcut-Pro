// FFmpeg wrapper implementation for ShotcutCPP
// Conditional compilation for graceful degradation when FFmpeg is not available

#include "video/ffmpeg_wrapper.h"
#include <iostream>
#include <algorithm>

namespace Shotcut::Video {

// CodecInfo implementation
CodecInfo CodecInfo::from_av_codec(const AVCodec* codec) {
    CodecInfo info;
    
    if (!codec) {
        return info;
    }
    
    info.name = codec->name ? codec->name : "unknown";
    info.long_name = codec->long_name ? codec->long_name : "Unknown Codec";
    info.codec_id = codec->id;
    info.hardware_accelerated = false; // Will be updated during detection
    info.can_decode = (codec->capabilities & AV_CODEC_CAP_DECODE) != 0;
    info.can_encode = (codec->capabilities & AV_CODEC_CAP_ENCODE) != 0;
    info.max_bitrate = 0; // Default, will be updated if available
    info.max_width = codec->max_lowres ? codec->max_lowres : 8192;
    info.max_height = codec->max_lowres ? codec->max_lowres : 8192;
    info.max_framerate = 60.0f; // Default assumption
    
    // Hardware acceleration defaults (will be updated during detection)
    info.supports_nvdec = false;
    info.supports_nvenc = false;
    info.supports_vaapi = false;
    info.supports_videotoolbox = false;
    info.supports_d3d11va = false;
    
    // Extract supported pixel formats
    if (codec->pix_fmts) {
        const enum AVPixelFormat* fmt = codec->pix_fmts;
        while (*fmt != AV_PIX_FMT_NONE) {
            info.pixel_formats.push_back(static_cast<PixelFormat>(*fmt));
            ++fmt;
        }
    }
    
    return info;
}

bool CodecInfo::supports_format(PixelFormat format) const {
    return std::find(pixel_formats.begin(), pixel_formats.end(), format) != pixel_formats.end();
}

bool CodecInfo::supports_resolution(int width, int height) const {
    return width <= max_width && height <= max_height;
}

// FFmpegDecoder implementation
FFmpegDecoder::FFmpegDecoder() 
    : codec_(nullptr) {
}

FFmpegDecoder::~FFmpegDecoder() {
    cleanup();
}

bool FFmpegDecoder::initialize(const std::string& file_path) {
    try {
#ifndef ENABLE_FFMPEG
        std::cout << "FFmpeg not available - using placeholder decoder" << std::endl;
        return true; // Placeholder implementation
#else
        // Open input file
        AVFormatContext* format_ctx = nullptr;
        if (avformat_open_input(&format_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
            throw FFmpegError("Failed to open input file: " + file_path);
        }
        
        format_context_.reset(format_ctx);
        
        // Find stream information
        if (avformat_find_stream_info(format_context_.get(), nullptr) < 0) {
            throw FFmpegError("Failed to find stream information");
        }
        
        // Find video stream
        if (!find_video_stream()) {
            throw FFmpegError("No video stream found in file");
        }
        
        // Get codec
        const AVCodec* codec = avcodec_find_decoder(
            format_context_->streams[video_stream_index_]->codecpar->codec_id
        );
        
        if (!codec) {
            throw FFmpegError("Unsupported codec");
        }
        
        codec_ = codec;
        
        // Initialize codec context
        if (!initialize_codec_context(codec)) {
            throw FFmpegError("Failed to initialize codec context");
        }
        
        last_frame_time_ = std::chrono::high_resolution_clock::now();
        
        std::cout << "Successfully initialized FFmpeg decoder for: " << file_path << std::endl;
        return true;
#endif
    } catch (const FFmpegError& e) {
        std::cerr << "FFmpeg initialization error: " << e.what() << std::endl;
        cleanup();
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error during FFmpeg initialization: " << e.what() << std::endl;
        cleanup();
        return false;
    }
}

bool FFmpegDecoder::initialize_with_hw_accel(const std::string& file_path, 
                                           const std::string& hw_device) {
#ifndef ENABLE_FFMPEG
    std::cout << "FFmpeg hardware acceleration not available - using placeholder" << std::endl;
    return true;
#else
    // Try hardware acceleration first, fallback to software
    use_nvdec_ = (hw_device == "auto" || hw_device == "nvdec") && 
                  CodecManager::get().is_nvdec_available();
    
    if (use_nvdec_ && !enable_nvdec()) {
        std::cout << "NVDEC failed, falling back to software decoding" << std::endl;
        use_nvdec_ = false;
    }
    
    return initialize(file_path);
#endif
}

void FFmpegDecoder::cleanup() {
    codec_ = nullptr;
    video_stream_index_ = -1;
    
    // Clear frame queue
    std::lock_guard<std::mutex> lock(frame_queue_mutex_);
    while (!frame_queue_.empty()) {
        frame_queue_.pop();
    }
    
    frames_decoded_ = 0;
    average_decode_time_ = 0.0;
}

bool FFmpegDecoder::is_initialized() const {
#ifndef ENABLE_FFMPEG
    return true; // Placeholder
#else
    return format_context_ && codec_ && video_stream_index_ >= 0;
#endif
}

std::optional<CodecInfo> FFmpegDecoder::get_codec_info() const {
    if (!codec_) {
        return std::nullopt;
    }
    
    return CodecInfo::from_av_codec(codec_);
}

int FFmpegDecoder::get_width() const {
#ifndef ENABLE_FFMPEG
    return 1920; // Placeholder
#else
    if (video_stream_index_ >= 0 && format_context_) {
        return format_context_->streams[video_stream_index_]->codecpar->width;
    }
    return 0;
#endif
}

int FFmpegDecoder::get_height() const {
#ifndef ENABLE_FFMPEG
    return 1080; // Placeholder
#else
    if (video_stream_index_ >= 0 && format_context_) {
        return format_context_->streams[video_stream_index_]->codecpar->height;
    }
    return 0;
#endif
}

AVPixelFormat FFmpegDecoder::get_pixel_format() const {
#ifndef ENABLE_FFMPEG
    return AV_PIX_FMT_YUV420P; // Placeholder
#else
    if (video_stream_index_ >= 0 && format_context_) {
        return static_cast<AVPixelFormat>(
            format_context_->streams[video_stream_index_]->codecpar->format
        );
    }
    return AV_PIX_FMT_NONE;
#endif
}

AVRational FFmpegDecoder::get_frame_rate() const {
#ifndef ENABLE_FFMPEG
    return {30, 1}; // Placeholder 30fps
#else
    if (video_stream_index_ >= 0 && format_context_) {
        return format_context_->streams[video_stream_index_]->avg_frame_rate;
    }
    return {0, 0};
#endif
}

int64_t FFmpegDecoder::get_duration() const {
#ifndef ENABLE_FFMPEG
    return 10000000; // Placeholder 10 seconds
#else
    if (format_context_) {
        return format_context_->duration;
    }
    return 0;
#endif
}

int64_t FFmpegDecoder::get_bitrate() const {
#ifndef ENABLE_FFMPEG
    return 5000000; // Placeholder 5Mbps
#else
    if (video_stream_index_ >= 0 && format_context_) {
        return format_context_->streams[video_stream_index_]->codecpar->bit_rate;
    }
    return 0;
#endif
}

std::optional<AVFramePtr> FFmpegDecoder::decode_next_frame() {
#ifndef ENABLE_FFMPEG
    // Placeholder implementation - simulate frame decode
    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Simulate 60fps
    frames_decoded_++;
    return std::nullopt;
#else
    auto start_time = std::chrono::high_resolution_clock::now();
    
    AVPacketPtr packet(nullptr, [](AVPacket* p) { if (p) av_packet_free(&p); });
    packet.reset(av_packet_alloc());
    
    if (!packet) {
        return std::nullopt;
    }
    
    int ret = av_read_frame(format_context_.get(), packet.get());
    if (ret < 0) {
        if (ret == AVERROR_EOF) {
            // End of file
            return std::nullopt;
        }
        throw FFmpegError("Error reading frame", ret);
    }
    
    // Only process video packets
    if (packet->stream_index != video_stream_index_) {
        return std::nullopt;
    }
    
    ret = avcodec_send_packet(codec_context_.get(), packet.get());
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        throw FFmpegError("Error sending packet to decoder", ret);
    }
    
    AVFramePtr frame(nullptr, [](AVFrame* f) { if (f) av_frame_free(&f); });
    frame.reset(av_frame_alloc());
    
    if (!frame) {
        throw FFmpegError("Failed to allocate frame");
    }
    
    ret = avcodec_receive_frame(codec_context_.get(), frame.get());
    if (ret < 0 && ret != AVERROR(EAGAIN)) {
        throw FFmpegError("Error receiving frame from decoder", ret);
    }
    
    if (ret == 0) {
        // Successfully decoded frame
        frames_decoded_++;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto decode_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time
        );
        
        update_performance_stats(decode_time);
        
        return frame;
    }
    
    return std::nullopt;
#endif
}

bool FFmpegDecoder::seek_to_timestamp(int64_t timestamp) {
#ifndef ENABLE_FFMPEG
    std::cout << "Placeholder seek to timestamp: " << timestamp << std::endl;
    return true;
#else
    if (!format_context_) {
        return false;
    }
    
    int64_t seek_target = timestamp;
    int stream_index = video_stream_index_;
    
    // Convert timestamp to stream time base
    if (stream_index >= 0) {
        AVRational time_base = format_context_->streams[stream_index]->time_base;
        seek_target = av_rescale_q(timestamp, {1, 1000000}, time_base);
    }
    
    int ret = av_seek_frame(format_context_.get(), stream_index, seek_target, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        throw FFmpegError("Failed to seek", ret);
    }
    
    // Flush decoder
    avcodec_flush_buffers(codec_context_.get());
    
    return true;
#endif
}

bool FFmpegDecoder::seek_to_frame(int64_t frame_number) {
#ifndef ENABLE_FFMPEG
    std::cout << "Placeholder seek to frame: " << frame_number << std::endl;
    return true;
#else
    AVRational frame_rate = get_frame_rate();
    if (frame_rate.num == 0 || frame_rate.den == 0) {
        return false;
    }
    
    int64_t timestamp = (frame_number * 1000000 * frame_rate.den) / frame_rate.num;
    return seek_to_timestamp(timestamp);
#endif
}

bool FFmpegDecoder::enable_nvdec() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // NVDEC implementation would go here
    use_nvdec_ = true;
    return true;
#endif
}

bool FFmpegDecoder::enable_vaapi() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // VAAPI implementation would go here
    use_vaapi_ = true;
    return true;
#endif
}

bool FFmpegDecoder::enable_videotoolbox() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // VideoToolbox implementation would go here
    use_videotoolbox_ = true;
    return true;
#endif
}

bool FFmpegDecoder::is_using_hw_accel() const {
    return use_nvdec_ || use_vaapi_ || use_videotoolbox_;
}

std::string FFmpegDecoder::get_hw_accel_type() const {
    if (use_nvdec_) return "NVDEC";
    if (use_vaapi_) return "VAAPI";
    if (use_videotoolbox_) return "VideoToolbox";
    return "Software";
}

double FFmpegDecoder::get_average_decode_time() const {
    return average_decode_time_;
}

uint64_t FFmpegDecoder::get_frames_decoded() const {
    return frames_decoded_;
}

void FFmpegDecoder::reset_performance_stats() {
    frames_decoded_ = 0;
    average_decode_time_ = 0.0;
    last_frame_time_ = std::chrono::high_resolution_clock::now();
}

// Private methods
bool FFmpegDecoder::find_video_stream() {
#ifndef ENABLE_FFMPEG
    return true; // Placeholder
#else
    for (unsigned int i = 0; i < format_context_->nb_streams; i++) {
        if (format_context_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index_ = i;
            return true;
        }
    }
    return false;
#endif
}

bool FFmpegDecoder::initialize_codec_context(const AVCodec* codec) {
#ifndef ENABLE_FFMPEG
    return true; // Placeholder
#else
    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        return false;
    }
    
    codec_context_.reset(codec_ctx, [](AVCodecContext* ctx) { 
        if (ctx) avcodec_free(&ctx); 
    });
    
    // Copy codec parameters from stream
    int ret = avcodec_parameters_to_context(
        codec_context_.get(), 
        format_context_->streams[video_stream_index_]->codecpar
    );
    
    if (ret < 0) {
        return false;
    }
    
    // Open codec
    ret = avcodec_open2(codec_context_.get(), codec, nullptr);
    if (ret < 0) {
        return false;
    }
    
    return true;
#endif
}

bool FFmpegDecoder::setup_hardware_acceleration() {
#ifndef ENABLE_FFMPEG
    return true; // Placeholder
#else
    // Hardware acceleration setup would go here
    return true;
#endif
}

AVFramePtr FFmpegDecoder::allocate_frame() {
#ifndef ENABLE_FFMPEG
    return AVFramePtr(nullptr, [](AVFrame*){}); // Placeholder
#else
    AVFrame* frame = av_frame_alloc();
    return AVFramePtr(frame, [](AVFrame* f) { if (f) av_frame_free(&f); });
#endif
}

void FFmpegDecoder::queue_frame(AVFramePtr frame) {
    std::lock_guard<std::mutex> lock(frame_queue_mutex_);
    frame_queue_.push(std::move(frame));
    frame_queue_cv_.notify_one();
}

bool FFmpegDecoder::convert_hw_frame_to_software(AVFrame* hw_frame) {
#ifndef ENABLE_FFMPEG
    return true; // Placeholder
#else
    // Hardware frame conversion would go here
    return true;
#endif
}

void FFmpegDecoder::update_performance_stats(std::chrono::nanoseconds decode_time) {
    double decode_time_ms = std::chrono::duration<double, std::milli>(decode_time).count();
    
    // Update rolling average
    if (frames_decoded_ == 1) {
        average_decode_time_ = decode_time_ms;
    } else {
        average_decode_time_ = (average_decode_time_ * 0.9) + (decode_time_ms * 0.1);
    }
    
    last_frame_time_ = std::chrono::high_resolution_clock::now();
}

// CodecManager implementation
std::unique_ptr<CodecManager> CodecManager::instance_ = nullptr;

CodecManager& CodecManager::get() {
    if (!instance_) {
        instance_ = std::make_unique<CodecManager>();
    }
    return *instance_;
}

bool CodecManager::initialize() {
    try {
        scan_available_codecs();
        detect_hardware_acceleration();
        
        std::cout << "CodecManager initialized with " << available_codecs_.size() 
                  << " available codecs" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CodecManager initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void CodecManager::cleanup() {
    available_codecs_.clear();
    codec_by_name_.clear();
    
    nvdec_available_ = false;
    nvenc_available_ = false;
    vaapi_available_ = false;
    videotoolbox_available_ = false;
    d3d11va_available_ = false;
}

std::optional<const CodecInfo*> CodecManager::get_codec_info(const std::string& name) const {
    auto it = codec_by_name_.find(name);
    if (it != codec_by_name_.end()) {
        return &it->second;
    }
    return std::nullopt;
}

std::optional<const CodecInfo*> CodecManager::get_codec_info(AVCodecID codec_id) const {
    for (const auto& codec : available_codecs_) {
        if (codec.codec_id == codec_id) {
            return &codec;
        }
    }
    return std::nullopt;
}

std::vector<CodecInfo> CodecManager::get_decoders_for_format(PixelFormat format) const {
    std::vector<CodecInfo> decoders;
    
    for (const auto& codec : available_codecs_) {
        if (codec.can_decode && codec.supports_format(format)) {
            decoders.push_back(codec);
        }
    }
    
    return decoders;
}

std::vector<CodecInfo> CodecManager::get_encoders_for_format(PixelFormat format) const {
    std::vector<CodecInfo> encoders;
    
    for (const auto& codec : available_codecs_) {
        if (codec.can_encode && codec.supports_format(format)) {
            encoders.push_back(codec);
        }
    }
    
    return encoders;
}

std::vector<std::string> CodecManager::get_available_hw_devices() const {
    std::vector<std::string> devices;
    
    if (nvdec_available_) devices.push_back("nvdec");
    if (nvenc_available_) devices.push_back("nvenc");
    if (vaapi_available_) devices.push_back("vaapi");
    if (videotoolbox_available_) devices.push_back("videotoolbox");
    if (d3d11va_available_) devices.push_back("d3d11va");
    
    return devices;
}

// Private methods
void CodecManager::scan_available_codecs() {
#ifndef ENABLE_FFMPEG
    // Placeholder codecs for testing
    CodecInfo h264;
    h264.name = "h264";
    h264.long_name = "H.264/AVC";
    h264.codec_id = AV_CODEC_ID_H264;
    h264.can_decode = true;
    h264.can_encode = true;
    h264.hardware_accelerated = false;
    available_codecs_.push_back(h264);
    codec_by_name_["h264"] = h264;
    
    CodecInfo h265;
    h265.name = "hevc";
    h265.long_name = "H.265/HEVC";
    h265.codec_id = AV_CODEC_ID_HEVC;
    h265.can_decode = true;
    h265.can_encode = true;
    h265.hardware_accelerated = false;
    available_codecs_.push_back(h265);
    codec_by_name_["hevc"] = h265;
    
    std::cout << "Using placeholder codec information" << std::endl;
#else
    const AVCodec* codec = nullptr;
    void* opaque = nullptr;
    
    while ((codec = av_codec_iterate(&opaque))) {
        CodecInfo info = CodecInfo::from_av_codec(codec);
        available_codecs_.push_back(info);
        codec_by_name_[info.name] = info;
    }
    
    std::cout << "Scanned " << available_codecs_.size() << " FFmpeg codecs" << std::endl;
#endif
}

void CodecManager::detect_hardware_acceleration() {
#ifndef ENABLE_FFMPEG
    std::cout << "FFmpeg not available - hardware acceleration detection skipped" << std::endl;
    return;
#else
    nvdec_available_ = test_nvdec_availability();
    nvenc_available_ = test_nvenc_availability();
    vaapi_available_ = test_vaapi_availability();
    videotoolbox_available_ = test_videotoolbox_availability();
    d3d11va_available_ = test_d3d11va_availability();
    
    std::cout << "Hardware acceleration detection:" << std::endl;
    std::cout << "  NVDEC: " << (nvdec_available_ ? "Available" : "Not available") << std::endl;
    std::cout << "  NVENC: " << (nvenc_available_ ? "Available" : "Not available") << std::endl;
    std::cout << "  VAAPI: " << (vaapi_available_ ? "Available" : "Not available") << std::endl;
    std::cout << "  VideoToolbox: " << (videotoolbox_available_ ? "Available" : "Not available") << std::endl;
    std::cout << "  D3D11VA: " << (d3d11va_available_ ? "Available" : "Not available") << std::endl;
#endif
}

bool CodecManager::test_nvdec_availability() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // NVDEC availability test would go here
    return false; // Placeholder
#endif
}

bool CodecManager::test_nvenc_availability() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // NVENC availability test would go here
    return false; // Placeholder
#endif
}

bool CodecManager::test_vaapi_availability() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // VAAPI availability test would go here
    return false; // Placeholder
#endif
}

bool CodecManager::test_videotoolbox_availability() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // VideoToolbox availability test would go here
    return false; // Placeholder
#endif
}

bool CodecManager::test_d3d11va_availability() {
#ifndef ENABLE_FFMPEG
    return false;
#else
    // D3D11VA availability test would go here
    return false; // Placeholder
#endif
}

} // namespace Shotcut::Video
