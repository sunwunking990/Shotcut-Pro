#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <coroutine>
#include <functional>
#include <variant>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

#ifdef ENABLE_VULKAN
#include <vulkan/vulkan.h>
#else
// Mock Vulkan types when not available
typedef void* VkDevice;
typedef void* VkImage;
typedef void* VkImageView;
typedef void* VkDeviceMemory;
typedef void* VkSampler;
typedef int VkFormat;
#define VK_NULL_HANDLE nullptr
#define VK_FORMAT_UNDEFINED 0
#endif

// Check if FFmpeg is available
#ifdef HAVE_FFMPEG
// Forward declarations for FFmpeg types
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
}
#else
// Mock definitions when FFmpeg is not available
typedef void AVCodec;
typedef void AVCodecContext;
typedef void AVFrame;
typedef void AVPacket;
typedef void SwsContext;
typedef void AVFormatContext;
typedef int AVPixelFormat;
typedef int AVCodecID;
typedef struct { int num; int den; } AVRational;
typedef int64_t int64_t;

#define AV_PIX_FMT_NONE 0
#define AV_PIX_FMT_YUV420P 1
#define AV_PIX_FMT_YUV444P 2
#define AV_PIX_FMT_RGB24 3
#define AV_PIX_FMT_RGBA 4
#define AV_PIX_FMT_BGRA 5
#define AV_PIX_FMT_NV12 6
#define AV_PIX_FMT_P010 7
#define AV_PIX_FMT_P016 8
#define AV_PIX_FMT_CUDA 9
#define AV_PIX_FMT_VULKAN 10
#endif

namespace Shotcut::Video {

// Modern C++20 error handling for FFmpeg
class FFmpegError : public std::runtime_error {
public:
    explicit FFmpegError(const std::string& message, int error_code = 0)
        : std::runtime_error(message), error_code_(error_code) {}
    
    int error_code() const noexcept { return error_code_; }
    
private:
    int error_code_;
};

// Safe RAII wrappers for FFmpeg objects
template<typename T, typename Deleter>
class FFmpegObject {
private:
    T* ptr_ = nullptr;
    Deleter deleter_;
    
public:
    explicit FFmpegObject(Deleter deleter) : deleter_(std::move(deleter)) {}
    
    ~FFmpegObject() {
        if (ptr_) {
            deleter_(ptr_);
        }
    }
    
    FFmpegObject(const FFmpegObject&) = delete;
    FFmpegObject& operator=(const FFmpegObject&) = delete;
    
    FFmpegObject(FFmpegObject&& other) noexcept 
        : ptr_(other.ptr_), deleter_(std::move(other.deleter_)) {
        other.ptr_ = nullptr;
    }
    
    FFmpegObject& operator=(FFmpegObject&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }
    
    T* get() const noexcept { return ptr_; }
    T* operator->() const noexcept { return ptr_; }
    
    T* release() noexcept {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }
    
    void reset(T* ptr = nullptr) noexcept {
        if (ptr_ && ptr_ != ptr) {
            deleter_(ptr_);
        }
        ptr_ = ptr;
    }
    
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
};

using AVFormatContextPtr = FFmpegObject<AVFormatContext, void(*)(AVFormatContext*)>;
using AVCodecContextPtr = FFmpegObject<AVCodecContext, void(*)(AVCodecContext*)>;
using AVFramePtr = FFmpegObject<AVFrame, void(*)(AVFrame*)>;
using AVPacketPtr = FFmpegObject<AVPacket, void(*)(AVPacket*)>;
using SwsContextPtr = FFmpegObject<SwsContext, void(*)(SwsContext*)>;

// Frame format specification
enum class PixelFormat : uint8_t {
    Unknown = AV_PIX_FMT_NONE,
    YUV420P = AV_PIX_FMT_YUV420P,
    YUV444P = AV_PIX_FMT_YUV444P,
    RGB24 = AV_PIX_FMT_RGB24,
    RGBA = AV_PIX_FMT_RGBA,
    BGRA = AV_PIX_FMT_BGRA,
    NV12 = AV_PIX_FMT_NV12,
    P010 = AV_PIX_FMT_P010,
    P016 = AV_PIX_FMT_P016,
    CUDA = AV_PIX_FMT_CUDA,
    VULKAN = AV_PIX_FMT_VULKAN
};

// Codec information
struct CodecInfo {
    std::string name;
    std::string long_name;
    AVCodecID codec_id;
    std::vector<PixelFormat> pixel_formats;
    std::vector<int> profiles;
    std::vector<int> levels;
    bool hardware_accelerated;
    bool can_decode;
    bool can_encode;
    int max_bitrate;
    int max_width;
    int max_height;
    float max_framerate;
    
    // Hardware acceleration capabilities
    bool supports_nvdec;
    bool supports_nvenc;
    bool supports_vaapi;
    bool supports_videotoolbox;
    bool supports_d3d11va;
    
    static CodecInfo from_av_codec(const AVCodec* codec);
    bool supports_format(PixelFormat format) const;
    bool supports_resolution(int width, int height) const;
};

class FFmpegDecoder {
private:
    AVFormatContextPtr format_context_;
    AVCodecContextPtr codec_context_;
    const AVCodec* codec_;
    int video_stream_index_ = -1;
    
    // Frame management
    std::queue<AVFramePtr> frame_queue_;
    std::mutex frame_queue_mutex_;
    std::condition_variable frame_queue_cv_;
    
    // Hardware acceleration
    bool use_nvdec_ = false;
    bool use_vaapi_ = false;
    bool use_videotoolbox_ = false;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    uint64_t frames_decoded_ = 0;
    double average_decode_time_ = 0.0;
    
public:
    explicit FFmpegDecoder();
    ~FFmpegDecoder();
    
    // Decoder initialization
    bool initialize(const std::string& file_path);
    bool initialize_with_hw_accel(const std::string& file_path, 
                               const std::string& hw_device = "auto");
    
    void cleanup();
    bool is_initialized() const;
    
    // Stream information
    std::optional<CodecInfo> get_codec_info() const;
    int get_width() const;
    int get_height() const;
    AVPixelFormat get_pixel_format() const;
    AVRational get_frame_rate() const;
    int64_t get_duration() const;
    int64_t get_bitrate() const;
    
    // Decoding
    std::optional<AVFramePtr> decode_next_frame();
    bool seek_to_timestamp(int64_t timestamp);
    bool seek_to_frame(int64_t frame_number);
    
    // Hardware acceleration
    bool enable_nvdec();
    bool enable_vaapi();
    bool enable_videotoolbox();
    bool is_using_hw_accel() const;
    
    std::string get_hw_accel_type() const;
    
    // Asynchronous decoding with futures
    std::future<AVFramePtr> decode_frame_async();
    std::future<std::vector<AVFramePtr>> decode_frames_async(size_t count);
    
    // Performance monitoring
    double get_average_decode_time() const { return average_decode_time_; }
    uint64_t get_frames_decoded() const { return frames_decoded_; }
    void reset_performance_stats();
    
private:
    bool find_video_stream();
    bool initialize_codec_context(const AVCodec* codec);
    bool setup_hardware_acceleration();
    
    AVFramePtr allocate_frame();
    void queue_frame(AVFramePtr frame);
    bool convert_hw_frame_to_software(AVFrame* hw_frame);
    
    void update_performance_stats(std::chrono::nanoseconds decode_time);
};

// Codec manager for system-wide codec information
class CodecManager {
private:
    static std::unique_ptr<CodecManager> instance_;
    std::vector<CodecInfo> available_codecs_;
    std::unordered_map<std::string, CodecInfo> codec_by_name_;
    
    // Hardware acceleration detection
    bool nvdec_available_ = false;
    bool nvenc_available_ = false;
    bool vaapi_available_ = false;
    bool videotoolbox_available_ = false;
    bool d3d11va_available_ = false;
    
public:
    static CodecManager& get() {
        if (!instance_) {
            instance_ = std::make_unique<CodecManager>();
        }
        return *instance_;
    }
    
    bool initialize();
    void cleanup();
    
    // Codec information
    const std::vector<CodecInfo>& get_available_codecs() const { return available_codecs_; }
    std::optional<const CodecInfo*> get_codec_info(const std::string& name) const;
    std::optional<const CodecInfo*> get_codec_info(AVCodecID codec_id) const;
    
    std::vector<CodecInfo> get_decoders_for_format(PixelFormat format) const;
    std::vector<CodecInfo> get_encoders_for_format(PixelFormat format) const;
    
    // Hardware acceleration
    bool is_nvdec_available() const { return nvdec_available_; }
    bool is_nvenc_available() const { return nvenc_available_; }
    bool is_vaapi_available() const { return vaapi_available_; }
    bool is_videotoolbox_available() const { return videotoolbox_available_; }
    bool is_d3d11va_available() const { return d3d11va_available_; }
    
    std::vector<std::string> get_available_hw_devices() const;
    
private:
    void scan_available_codecs();
    void detect_hardware_acceleration();
    
    bool test_nvdec_availability();
    bool test_nvenc_availability();
    bool test_vaapi_availability();
    bool test_videotoolbox_availability();
    bool test_d3d11va_availability();
};

} // namespace Shotcut::Video
