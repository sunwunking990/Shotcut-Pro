# Phase 6: Video Processing Pipeline Implementation Guide - Shotcut C++ Transformation

**Document Created:** November 13, 2025  
**Implementation Focus:** Building Hardware-Accelerated Video Processing System  
**Duration:** Months 7-10 (Video Pipeline Phase)  
**Prerequisites:** Phase 5 completed (UI Framework Development)

---

## PHASE 6 OVERVIEW

Phase 6 focuses on creating a professional-grade video processing pipeline that rivals commercial editors like Adobe Premiere Pro and DaVinci Resolve. This pipeline will be built entirely in modern C++20/23 with GPU acceleration through Vulkan and NVIDIA Video Codec SDK, providing real-time video processing, hardware acceleration, and the performance required for professional video editing workflows.

**Critical Success Factors:**
- **Hardware-First Architecture:** NVIDIA NVENC/NVDEC and CUDA acceleration
- **Real-Time Performance:** Smooth 60fps preview even with 4K footage
- **Modern C++20/23 Integration:** Modules, coroutines, and concepts throughout
- **Professional Codec Support:** H.264, H.265, AV1 with hardware acceleration
- **GPU Compute Shaders:** Custom effects processing on GPU
- **Memory Efficiency:** Smart frame buffering and GPU memory management

---

## 6.1 FFMPEG INTEGRATION FOR CODEC SUPPORT

### 6.1.1 Modern FFmpeg Integration Architecture

**FFmpeg Integration Philosophy:**
- **Custom Wrapper:** Security-hardened FFmpeg integration
- **Hardware Acceleration:** Automatic GPU decode/encode when available
- **Zero-Copy Workflows:** GPU-to-GPU memory transfers where possible
- **Frame Pooling:** Efficient frame buffer management
- **Error Resilience:** Robust error handling and recovery

**src/video/ffmpeg_wrapper.h:**
```cpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <chrono>
#include <coroutine>
#include <functional>

// Forward declarations for FFmpeg types
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
}

#include <vulkan/vulkan.h>
#include <cuda_runtime.h>
#include <nvcuvid.h>
#include <nvencodeapi.h>

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
    AVCodecPtr codec_;
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
    
    // Asynchronous decoding with C++20 coroutines
    std::task<AVFramePtr> decode_frame_async();
    std::task<std::vector<AVFramePtr>> decode_frames_async(size_t count);
    
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

class FFmpegEncoder {
private:
    AVFormatContextPtr format_context_;
    AVCodecContextPtr codec_context_;
    AVCodecPtr codec_;
    AVStream* video_stream_ = nullptr;
    
    // Encoding parameters
    int width_ = 1920;
    int height_ = 1080;
    AVPixelFormat pixel_format_ = AV_PIX_FMT_YUV420P;
    int64_t bitrate_ = 5000000; // 5 Mbps
    AVRational frame_rate_ = {25, 1};
    
    // Hardware acceleration
    bool use_nvenc_ = false;
    NV_ENC_BUFFER_FORMAT nvenc_format_ = NV_ENC_BUFFER_FORMAT_YUV420;
    GUID nvenc_preset_ = NV_ENC_PRESET_DEFAULT_GUID;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    uint64_t frames_encoded_ = 0;
    double average_encode_time_ = 0.0;
    
public:
    explicit FFmpegEncoder();
    ~FFmpegEncoder();
    
    // Encoder initialization
    bool initialize(const std::string& output_path);
    bool initialize_with_hw_accel(const std::string& output_path, 
                               const std::string& hw_device = "auto");
    
    void cleanup();
    bool is_initialized() const;
    
    // Encoding parameters
    void set_resolution(int width, int height);
    void set_pixel_format(PixelFormat format);
    void set_bitrate(int64_t bitrate);
    void set_frame_rate(const AVRational& frame_rate);
    void set_codec(AVCodecID codec_id);
    
    // Hardware acceleration
    bool enable_nvenc(NV_ENC_PRESET preset = NV_ENC_PRESET_DEFAULT_GUID);
    bool enable_vaapi();
    bool is_using_hw_accel() const;
    
    std::string get_hw_accel_type() const;
    
    // Encoding
    bool encode_frame(const AVFrame* frame);
    bool flush_encoder();
    
    // Asynchronous encoding with C++20 coroutines
    std::task<bool> encode_frame_async(const AVFrame* frame);
    std::task<bool> encode_frames_async(std::span<AVFrame*> frames);
    
    // Performance monitoring
    double get_average_encode_time() const { return average_encode_time_; }
    uint64_t get_frames_encoded() const { return frames_encoded_; }
    void reset_performance_stats();
    
private:
    bool setup_output_format(const std::string& output_path);
    bool initialize_video_stream(const AVCodec* codec);
    bool setup_hardware_acceleration();
    
    bool configure_nvenc();
    bool configure_vaapi();
    
    void update_performance_stats(std::chrono::nanoseconds encode_time);
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
```

### 6.1.2 Hardware-Accelerated Decoding Implementation

**src/video/hardware_decoder.h:**
```cpp
#pragma once

#include "ffmpeg_wrapper.h"
#include <cuda_runtime.h>
#include <nvcuvid.h>
#include <vulkan/vulkan.h>
#include <memory>

namespace Shotcut::Video {

class HardwareDecoder {
private:
    CUcontext cuda_context_;
    CUdevice cuda_device_;
    CUvideodecoder video_decoder_;
    CUvideoparser video_parser_;
    
    // Frame management
    struct CUDAFrame {
        CUdeviceptr device_ptr;
        CUgraphicsResource graphics_resource;
        void* mapped_ptr;
        uint32_t width, height;
        NV_ENC_OUTPUT_PTR output_ptr;
        size_t output_size;
    };
    
    std::queue<std::unique_ptr<CUDAFrame>> frame_pool_;
    std::vector<CUVIDDECODEDISPINFO> display_info_queue_;
    
    // Vulkan interop
    VkDevice vulkan_device_;
    VkQueue vulkan_queue_;
    std::vector<VkExternalMemoryHandleTypePropertiesKHR> external_memory_types_;
    
public:
    HardwareDecoder();
    ~HardwareDecoder();
    
    // Initialization
    bool initialize(const CodecInfo& codec_info);
    void cleanup();
    
    // Decoding
    bool decode_packet(const AVPacket* packet);
    std::optional<std::unique_ptr<CUDAFrame>> get_decoded_frame();
    bool flush_decoder();
    
    // Vulkan integration
    VkImage get_vulkan_image(const CUDAFrame& frame);
    bool convert_to_vulkan_format(const CUDAFrame& frame, VkImage dst_image);
    
private:
    bool setup_cuda_context();
    bool setup_vulkan_interop();
    
    CUVIDDECODECREATEINFO get_decoder_create_info(const CodecInfo& codec_info);
    
    static int CUDAAPI_HANDLE_VIDEO_SEQUENCE(void* user_data, CUVIDEOFORMAT* format);
    static int CUDAAPI_HANDLE_PICTURE_DISPLAY(void* user_data, CUVIDPARSERDISPINFO* disp_info);
    static int CUDAAPI_HANDLE_PICTURE_DECODE(void* user_data, CUVIDPICPARAMS* pic_params);
};

// Hardware decoder factory
class HardwareDecoderFactory {
public:
    static std::unique_ptr<HardwareDecoder> create_decoder(const CodecInfo& codec_info);
    static bool supports_hardware_decode(const CodecInfo& codec_info);
    
private:
    static bool is_nvidia_gpu_available();
    static bool is_intel_gpu_available();
    static bool is_amd_gpu_available();
};

} // namespace Shotcut::Video
```

---

## 6.2 NVIDIA VIDEO CODEC SDK INTEGRATION

### 6.2.1 NVIDIA NVENC/NVDEC Integration

**NVIDIA SDK Integration Philosophy:**
- **Direct GPU Control:** Bypass FFmpeg for critical paths
- **Zero-Copy Pipelines:** GPU-to-GPU memory transfers
- **Multi-Stream Support:** Concurrent encoding/decoding
- **Performance Optimization:** GPU memory pools and async operations
- **Error Resilience:** Comprehensive error handling and recovery

**src/video/nvidia_video_sdk.h:**
```cpp
#pragma once

#include <cuda_runtime.h>
#include <nvEncodeAPI.h>
#include <nvcuvid.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <functional>

namespace Shotcut::Video {

// NVIDIA encoder presets
enum class NVENCPreset : uint8_t {
    Default = NV_ENC_PRESET_DEFAULT_GUID,
    HP = NV_ENC_PRESET_HP_GUID,
    HQ = NV_ENC_PRESET_HQ_GUID,
    LowLatency = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID,
    LowLatencyHQ = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID,
    LosslessHP = NV_ENC_PRESET_LOSSLESS_HP_GUID,
    LosslessHP5 = NV_ENC_PRESET_LOSSLESS_HP_5_GUID
};

// NVIDIA encoder configuration
struct NVENCConfig {
    int width = 1920;
    int height = 1080;
    NV_ENC_BUFFER_FORMAT buffer_format = NV_ENC_BUFFER_FORMAT_YUV420;
    NV_ENC_OUTPUT_FORMAT output_format = NV_ENC_OUTPUT_FORMAT_H264;
    
    // Rate control
    NV_ENC_PARAMS_RC_MODE rate_control_mode = NV_ENC_PARAMS_RC_VBR;
    uint32_t target_bitrate = 5000000; // 5 Mbps
    uint32_t max_bitrate = 8000000; // 8 Mbps
    uint32_t vbv_buffer_size = 0; // Auto
    uint32_t vbv_initial_delay = 0; // Auto
    
    // Frame rate
    uint32_t frame_rate_num = 30;
    uint32_t frame_rate_den = 1;
    
    // GOP structure
    uint32_t gop_length = 30; // 1 second at 30fps
    uint32_t b_frame_count = 0; // No B-frames for low latency
    
    // Quality
    NV_ENC_QUALITY preset = NV_ENC_QUALITY_HIGH_PERFORMANCE;
    
    // Multi-pass
    bool enable_two_pass = false;
    NV_ENC_TWO_PASS_GUID two_pass_guid = NV_ENC_TWO_PASS_GUID;
    
    // Psycho visual tuning
    NV_ENC_TUNING_INFO tuning_info = {NV_ENC_TUNING_HIGH_QUALITY, 0};
    
    // Adaptive quantization
    bool enable_adaptive_quantization = true;
    
    // Deblocking filter
    bool enable_deblocking_filter = true;
    
    // Lookahead
    uint32_t lookahead = 10; // frames
    
    static NVENCConfig create_default() { return {}; }
    static NVENCConfig create_high_quality();
    static NVENCConfig create_low_latency();
};

class NVENCEncoder {
private:
    NV_ENCODE_API_FUNCTION_LIST* nvenc_api_;
    void* encoder_interface_ = nullptr;
    NV_ENC_INITIALIZE_PARAMS init_params_;
    
    // CUDA context
    CUcontext cuda_context_;
    CUdevice cuda_device_;
    int cuda_device_id_ = 0;
    
    // Buffer management
    struct InputBuffer {
        CUdeviceptr device_ptr;
        void* host_ptr;
        size_t size;
        NV_ENC_REGISTERED_PTR registered_ptr;
        bool is_mapped;
    };
    
    std::vector<InputBuffer> input_buffers_;
    std::queue<uint32_t> available_buffer_indices_;
    std::mutex buffer_mutex_;
    
    // Output bitstream
    std::vector<uint8_t> output_buffer_;
    std::mutex output_mutex_;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point start_time_;
    uint64_t frames_encoded_ = 0;
    double average_encode_time_ = 0.0;
    uint32_t current_fps_ = 0;
    
public:
    NVENCEncoder();
    ~NVENCEncoder();
    
    // Initialization
    bool initialize(const NVENCConfig& config, int cuda_device_id = 0);
    void cleanup();
    bool is_initialized() const;
    
    // Encoding
    bool encode_frame(const uint8_t* yuv_data, size_t frame_size);
    bool flush_encoder();
    std::vector<uint8_t> get_encoded_data();
    
    // Configuration
    void update_bitrate(uint32_t target_bitrate, uint32_t max_bitrate = 0);
    void update_frame_rate(uint32_t fps_num, uint32_t fps_den);
    void update_resolution(int width, int height);
    
    // Asynchronous encoding
    std::task<bool> encode_frame_async(const uint8_t* yuv_data, size_t frame_size);
    std::task<std::vector<uint8_t>> flush_encoder_async();
    
    // Status and statistics
    NV_ENC_INITIALIZE_PARAMS get_init_params() const { return init_params_; }
    uint64_t get_frames_encoded() const { return frames_encoded_; }
    double get_average_encode_time() const { return average_encode_time_; }
    uint32_t get_current_fps() const { return current_fps_; }
    
    // Hardware capabilities
    static bool is_available();
    static std::vector<int> get_available_cuda_devices();
    static NVENC_CAPS get_encoder_caps(NV_ENC_CODEC codec);
    
private:
    bool load_nvenc_library();
    bool setup_cuda_context(int device_id);
    bool create_encoder_interface(const NVENCConfig& config);
    bool setup_input_buffers(size_t buffer_size, size_t buffer_count = 8);
    
    void cleanup_cuda_context();
    void cleanup_encoder_interface();
    void cleanup_input_buffers();
    
    bool register_input_buffer(InputBuffer& buffer, const uint8_t* data, size_t size);
    void unregister_input_buffer(InputBuffer& buffer);
    
    void update_performance_stats(std::chrono::nanoseconds encode_time);
    
    static void CUDAAPI HandleBitstream(void* user_data, NV_ENC_OUTPUT_PTR bitstream_ptr);
    static void CUDAAPI HandleCompletionEvent(void* user_data, NV_ENC_OUTPUT_PTR completion_event);
};

// NVDEC decoder for hardware decoding
class NVDECDecoder {
private:
    CUvideodecoder video_decoder_;
    CUvideoparser video_parser_;
    CUcontext cuda_context_;
    CUdevice cuda_device_;
    
    // Frame management
    struct DecodedFrame {
        CUdeviceptr device_ptr;
        CUgraphicsResource vulkan_resource;
        VkImage vulkan_image;
        VkImageView vulkan_image_view;
        uint32_t width, height;
        NV_ENC_OUTPUT_PTR bitstream_ptr;
        bool is_available;
    };
    
    std::vector<std::unique_ptr<DecodedFrame>> frame_pool_;
    std::queue<size_t> available_frame_indices_;
    std::mutex frame_mutex_;
    
    // Vulkan interop
    VkDevice vulkan_device_;
    VkQueue vulkan_queue_;
    uint32_t vulkan_queue_family_;
    
public:
    NVDECDecoder();
    ~NVDECDecoder();
    
    // Initialization
    bool initialize(const CodecInfo& codec_info, VkDevice vulkan_device);
    void cleanup();
    
    // Decoding
    bool decode_packet(const uint8_t* packet_data, size_t packet_size);
    std::optional<VkImage> get_decoded_frame();
    bool flush_decoder();
    
    // Status
    bool is_initialized() const;
    uint32_t get_frame_count() const;
    
private:
    bool setup_cuda_context();
    bool create_video_decoder(const CodecInfo& codec_info);
    bool setup_vulkan_interop(VkDevice vulkan_device);
    
    CUVIDDECODECREATEINFO get_decoder_create_info(const CodecInfo& codec_info);
    
    bool allocate_frame_pool(size_t frame_count = 16);
    std::unique_ptr<DecodedFrame> get_available_frame();
    
    static int CUDAAPI HandleVideoSequence(void* user_data, CUVIDEOFORMAT* format);
    static int CUDAAPI HandlePictureDisplay(void* user_data, CUVIDPARSERDISPINFO* disp_info);
    static int CUDAAPI HandlePictureDecode(void* user_data, CUVIDPICPARAMS* pic_params);
};

// NVIDIA Video SDK manager
class NVIDIAVideoSDKManager {
private:
    static std::unique_ptr<NVIDIAVideoSDKManager> instance_;
    
    bool nvenc_available_ = false;
    bool nvdec_available_ = false;
    std::vector<int> available_cuda_devices_;
    NV_ENCODE_API_FUNCTION_LIST* nvenc_api_ = nullptr;
    
    // Driver and SDK versions
    uint32_t driver_version_ = 0;
    uint32_t nvenc_version_ = 0;
    uint32_t cuda_runtime_version_ = 0;
    
public:
    static NVIDIAVideoSDKManager& get() {
        if (!instance_) {
            instance_ = std::make_unique<NVIDIAVideoSDKManager>();
        }
        return *instance_;
    }
    
    bool initialize();
    void cleanup();
    
    // Capabilities
    bool is_nvenc_available() const { return nvenc_available_; }
    bool is_nvdec_available() const { return nvdec_available_; }
    
    const std::vector<int>& get_available_cuda_devices() const { 
        return available_cuda_devices_; 
    }
    
    // Version information
    uint32_t get_driver_version() const { return driver_version_; }
    uint32_t get_nvenc_version() const { return nvenc_version_; }
    uint32_t get_cuda_runtime_version() const { return cuda_runtime_version_; }
    
    std::string get_version_string() const;
    
    // Factory methods
    std::unique_ptr<NVENCEncoder> create_encoder(const NVENCConfig& config, int device_id = 0);
    std::unique_ptr<NVDECDecoder> create_decoder(const CodecInfo& codec_info, VkDevice vulkan_device);
    
private:
    void detect_nvenc_availability();
    void detect_nvdec_availability();
    void enumerate_cuda_devices();
    void query_driver_versions();
    
    bool load_nvenc_library();
    bool test_nvenc_functionality();
};

} // namespace Shotcut::Video
```

---

## 6.3 FRAME BUFFER MANAGEMENT SYSTEM

### 6.3.1 Efficient Frame Buffer Architecture

**Frame Buffer Design Philosophy:**
- **GPU Memory Pools:** Pre-allocated GPU memory for video frames
- **Zero-Copy Operations:** Minimize CPU-GPU memory transfers
- **Smart Caching:** LRU cache for recently used frames
- **Memory Pressure Handling:** Automatic cleanup under memory pressure
- **Multi-Format Support:** Various pixel formats and color spaces

**src/video/frame_buffer.h:**
```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <cuda_runtime.h>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

namespace Shotcut::Video {

// Frame descriptor for efficient allocation
struct FrameDescriptor {
    uint32_t width;
    uint32_t height;
    PixelFormat pixel_format;
    size_t size_bytes;
    uint32_t alignment;
    
    // Hash for container operations
    size_t hash() const noexcept {
        return std::hash<uint64_t>{}(
            (static_cast<uint64_t>(width) << 32) | static_cast<uint64_t>(height)
        ) ^ std::hash<uint32_t>{}(static_cast<uint32_t>(pixel_format));
    }
    
    bool operator==(const FrameDescriptor& other) const noexcept {
        return width == other.width && height == other.height && 
               pixel_format == other.pixel_format;
    }
};

// GPU frame with Vulkan/CUDA interop
class GPUFrame {
private:
    VkImage vulkan_image_;
    VkImageView vulkan_image_view_;
    VkDeviceMemory vulkan_memory_;
    VkSampler sampler_;
    
    // CUDA interop
    CUdeviceptr cuda_device_ptr_;
    CUgraphicsResource cuda_graphics_resource_;
    void* mapped_cuda_ptr_;
    
    // Frame properties
    FrameDescriptor descriptor_;
    bool is_mapped_ = false;
    bool is_in_use_ = false;
    std::chrono::high_resolution_clock::time_point last_access_time_;
    
    // Reference counting
    std::atomic<uint32_t> ref_count_;
    
public:
    GPUFrame(const FrameDescriptor& desc);
    ~GPUFrame();
    
    // Frame properties
    const FrameDescriptor& descriptor() const { return descriptor_; }
    uint32_t width() const { return descriptor_.width; }
    uint32_t height() const { return descriptor_.height; }
    PixelFormat pixel_format() const { return descriptor_.pixel_format; }
    
    // Vulkan access
    VkImage get_vulkan_image() const { return vulkan_image_; }
    VkImageView get_vulkan_image_view() const { return vulkan_image_view_; }
    VkSampler get_sampler() const { return sampler_; }
    
    // CUDA access
    CUdeviceptr get_cuda_device_ptr() const { return cuda_device_ptr_; }
    void* get_mapped_cuda_ptr() const { return mapped_cuda_ptr_; }
    
    // Memory management
    bool map_cuda_memory();
    bool unmap_cuda_memory();
    bool map_vulkan_memory();
    bool unmap_vulkan_memory();
    
    // Reference counting
    void add_ref() { ref_count_.fetch_add(1, std::memory_order_relaxed); }
    void release();
    uint32_t ref_count() const { return ref_count_.load(std::memory_order_acquire); }
    
    // Usage tracking
    void mark_in_use() { 
        is_in_use_ = true;
        last_access_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void mark_unused() { is_in_use_ = false; }
    bool is_in_use() const { return is_in_use_; }
    
    std::chrono::high_resolution_clock::time_point last_access_time() const { 
        return last_access_time_; 
    }
    
private:
    bool create_vulkan_resources();
    bool create_cuda_interop();
    void cleanup_vulkan_resources();
    void cleanup_cuda_resources();
};

// Frame pool with smart caching
class FramePool {
private:
    struct PoolBucket {
        FrameDescriptor descriptor;
        std::queue<std::unique_ptr<GPUFrame>> available_frames;
        std::vector<std::unique_ptr<GPUFrame>> all_frames;
        std::mutex bucket_mutex;
        size_t max_pool_size = 16;
        size_t min_pool_size = 2;
        
        // Statistics
        size_t total_allocated = 0;
        size_t peak_usage = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
    };
    
    std::unordered_map<size_t, std::unique_ptr<PoolBucket>> frame_pools_;
    std::mutex pools_mutex_;
    
    // Global memory tracking
    size_t total_gpu_memory_used_ = 0;
    size_t max_gpu_memory_allowed_;
    VkDevice vulkan_device_;
    CUcontext cuda_context_;
    
    // Cleanup thread
    std::thread cleanup_thread_;
    std::atomic<bool> cleanup_running_;
    std::chrono::seconds cleanup_interval_{std::chrono::seconds(5)};
    
public:
    FramePool(VkDevice vulkan_device, CUcontext cuda_context, 
              size_t max_gpu_memory = 2ULL * 1024 * 1024 * 1024); // 2GB
    ~FramePool();
    
    // Frame allocation
    std::shared_ptr<GPUFrame> allocate_frame(const FrameDescriptor& descriptor);
    std::shared_ptr<GPUFrame> get_frame(const FrameDescriptor& descriptor);
    void return_frame(std::shared_ptr<GPUFrame> frame);
    
    // Pool management
    void set_pool_size_limits(const FrameDescriptor& descriptor, 
                           size_t min_size, size_t max_size);
    void preload_common_formats();
    
    // Memory management
    size_t get_total_memory_used() const { return total_gpu_memory_used_; }
    size_t get_max_memory_allowed() const { return max_gpu_memory_allowed_; }
    float get_memory_usage_percentage() const;
    
    // Statistics
    PoolBucket::Statistics get_pool_statistics(const FrameDescriptor& descriptor) const;
    std::vector<PoolBucket::Statistics> get_all_statistics() const;
    
    void reset_statistics();
    
    // Memory pressure handling
    void set_memory_pressure_callback(std::function<void(float)> callback);
    void trigger_cleanup();
    
private:
    PoolBucket* get_or_create_bucket(const FrameDescriptor& descriptor);
    std::unique_ptr<GPUFrame> create_new_frame(const FrameDescriptor& descriptor);
    
    void cleanup_unused_frames();
    void cleanup_thread_function();
    
    bool can_allocate_frame(const FrameDescriptor& descriptor) const;
    void update_memory_usage(size_t delta);
    
    static size_t calculate_frame_size(const FrameDescriptor& descriptor);
    static VkFormat pixel_format_to_vulkan(PixelFormat format);
};

// Frame cache for efficient frame reuse
class FrameCache {
private:
    struct CacheEntry {
        std::shared_ptr<GPUFrame> frame;
        std::chrono::high_resolution_clock::time_point access_time;
        std::string key;
        
        CacheEntry(std::shared_ptr<GPUFrame> f, std::string k)
            : frame(std::move(f)), access_time(std::chrono::high_resolution_clock::now()), 
              key(std::move(k)) {}
    };
    
    std::unordered_map<std::string, CacheEntry> cache_;
    std::list<std::string> lru_list_;
    std::mutex cache_mutex_;
    
    size_t max_cache_size_ = 100;
    size_t current_size_ = 0;
    
    // Statistics
    size_t hits_ = 0;
    size_t misses_ = 0;
    
public:
    explicit FrameCache(size_t max_size = 100);
    ~FrameCache() = default;
    
    // Cache operations
    std::shared_ptr<GPUFrame> get(const std::string& key);
    void put(const std::string& key, std::shared_ptr<GPUFrame> frame);
    void remove(const std::string& key);
    void clear();
    
    // Cache configuration
    void set_max_size(size_t max_size);
    size_t get_max_size() const { return max_cache_size_; }
    size_t get_current_size() const { return current_size_; }
    
    // Statistics
    size_t get_hits() const { return hits_; }
    size_t get_misses() const { return misses_; }
    double get_hit_rate() const;
    void reset_statistics();
    
    // Predefined keys
    static std::string make_frame_key(const std::string& file_path, int64_t timestamp);
    static std::string make_effect_key(const std::string& effect_id, 
                                   const std::vector<std::string>& parameters);
    
private:
    void update_lru_order(const std::string& key);
    void evict_oldest();
};

} // namespace Shotcut::Video
```

---

## 6.4 HARDWARE-ACCELERATED DECODING

### 6.4.1 Multi-Platform Hardware Decoding

**Hardware Decoding Philosophy:**
- **Platform Abstraction:** Unified API across Windows/Linux
- **Automatic Fallback:** Software decoding when hardware unavailable
- **Format Support:** H.264, H.265, AV1, VP9, etc.
- **Zero-Copy Workflows:** Direct GPU memory transfers
- **Performance Monitoring:** Real-time decode performance tracking

**src/video/hardware_decoding.h:**
```cpp
#pragma once

#include "frame_buffer.h"
#include <memory>
#include <vector>
#include <functional>

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi1_2.h>
#endif

#ifdef __linux__
#include <va/va.h>
#include <va/va_drmcommon.h>
#endif

namespace Shotcut::Video {

// Hardware decoding platforms
enum class HWDecodePlatform : uint8_t {
    None,
    NVIDIA_NVDEC,
    Intel_QSV,  // Quick Sync Video
    AMD_VCE,    // Video Coding Engine
    VA_API,      // Video Acceleration API (Linux)
    D3D11VA,     // Direct3D 11 Video Acceleration (Windows)
    VideoToolbox  // Apple Video Toolbox (macOS)
};

// Hardware decoder capabilities
struct HWDecoderCaps {
    HWDecodePlatform platform;
    std::vector<AVCodecID> supported_codecs;
    std::vector<PixelFormat> supported_formats;
    std::pair<int, int> max_resolution; // width, height
    float max_framerate;
    int max_concurrent_streams;
    bool supports_10bit;
    bool supports_hdr;
    bool supports_zero_copy;
    
    static HWDecoderCaps detect_capabilities();
};

// Platform-specific hardware decoder base class
class PlatformHWDecoder {
public:
    virtual ~PlatformHWDecoder() = default;
    
    virtual bool initialize(const CodecInfo& codec_info) = 0;
    virtual void cleanup() = 0;
    virtual bool is_initialized() const = 0;
    
    virtual bool decode_packet(const uint8_t* data, size_t size) = 0;
    virtual std::shared_ptr<GPUFrame> get_decoded_frame() = 0;
    virtual bool flush_decoder() = 0;
    
    virtual HWDecodePlatform get_platform() const = 0;
    virtual std::string get_platform_name() const = 0;
    
    // Capabilities
    virtual bool supports_codec(AVCodecID codec_id) const = 0;
    virtual bool supports_format(PixelFormat format) const = 0;
    virtual bool supports_resolution(int width, int height) const = 0;
    
protected:
    CodecInfo codec_info_;
    bool initialized_ = false;
};

// NVIDIA NVDEC implementation
class NVDECDecoder : public PlatformHWDecoder {
private:
    std::unique_ptr<NVDECDecoder> nvdec_impl_;
    
public:
    NVDECDecoder();
    ~NVDECDecoder() override;
    
    bool initialize(const CodecInfo& codec_info) override;
    void cleanup() override;
    bool is_initialized() const override;
    
    bool decode_packet(const uint8_t* data, size_t size) override;
    std::shared_ptr<GPUFrame> get_decoded_frame() override;
    bool flush_decoder() override;
    
    HWDecodePlatform get_platform() const override { 
        return HWDecodePlatform::NVIDIA_NVDEC; 
    }
    
    std::string get_platform_name() const override { return "NVIDIA NVDEC"; }
    
    bool supports_codec(AVCodecID codec_id) const override;
    bool supports_format(PixelFormat format) const override;
    bool supports_resolution(int width, int height) const override;
};

// Intel Quick Sync Video implementation
class QSVDecoder : public PlatformHWDecoder {
private:
    // Intel Media SDK implementation would go here
    void* qsv_session_;
    
public:
    QSVDecoder();
    ~QSVDecoder() override;
    
    bool initialize(const CodecInfo& codec_info) override;
    void cleanup() override;
    bool is_initialized() const override;
    
    bool decode_packet(const uint8_t* data, size_t size) override;
    std::shared_ptr<GPUFrame> get_decoded_frame() override;
    bool flush_decoder() override;
    
    HWDecodePlatform get_platform() const override { 
        return HWDecodePlatform::Intel_QSV; 
    }
    
    std::string get_platform_name() const override { return "Intel Quick Sync Video"; }
    
    bool supports_codec(AVCodecID codec_id) const override;
    bool supports_format(PixelFormat format) const override;
    bool supports_resolution(int width, int height) const override;
};

// VA API implementation (Linux)
class VAAAPIDecoder : public PlatformHWDecoder {
private:
    VADisplay* va_display_;
    VAConfigID va_config_;
    VAContextID va_context_;
    
public:
    VAAAPIDecoder();
    ~VAAPIIDecoder() override;
    
    bool initialize(const CodecInfo& codec_info) override;
    void cleanup() override;
    bool is_initialized() const override;
    
    bool decode_packet(const uint8_t* data, size_t size) override;
    std::shared_ptr<GPUFrame> get_decoded_frame() override;
    bool flush_decoder() override;
    
    HWDecodePlatform get_platform() const override { 
        return HWDecodePlatform::VA_API; 
    }
    
    std::string get_platform_name() const override { return "VA API"; }
    
    bool supports_codec(AVCodecID codec_id) const override;
    bool supports_format(PixelFormat format) const override;
    bool supports_resolution(int width, int height) const override;
};

// D3D11VA implementation (Windows)
class D3D11VADecoder : public PlatformHWDecoder {
private:
    ID3D11Device* d3d11_device_;
    ID3D11VideoDevice* video_device_;
    ID3D11VideoDecoder* video_decoder_;
    
public:
    D3D11VADecoder();
    ~D3D11VADecoder() override;
    
    bool initialize(const CodecInfo& codec_info) override;
    void cleanup() override;
    bool is_initialized() const override;
    
    bool decode_packet(const uint8_t* data, size_t size) override;
    std::shared_ptr<GPUFrame> get_decoded_frame() override;
    bool flush_decoder() override;
    
    HWDecodePlatform get_platform() const override { 
        return HWDecodePlatform::D3D11VA; 
    }
    
    std::string get_platform_name() const override { return "Direct3D 11 Video"; }
    
    bool supports_codec(AVCodecID codec_id) const override;
    bool supports_format(PixelFormat format) const override;
    bool supports_resolution(int width, int height) const override;
};

// Unified hardware decoder manager
class HWDecoderManager {
private:
    static std::unique_ptr<HWDecoderManager> instance_;
    std::vector<std::unique_ptr<PlatformHWDecoder>> available_decoders_;
    HWDecoderCaps system_caps_;
    std::unique_ptr<PlatformHWDecoder> active_decoder_;
    
    // Fallback decoder
    std::unique_ptr<FFmpegDecoder> software_decoder_;
    
public:
    static HWDecoderManager& get() {
        if (!instance_) {
            instance_ = std::make_unique<HWDecoderManager>();
        }
        return *instance_;
    }
    
    bool initialize();
    void cleanup();
    
    // Decoder selection
    std::unique_ptr<PlatformHWDecoder> create_decoder(const CodecInfo& codec_info);
    bool select_best_decoder(const CodecInfo& codec_info);
    
    // Decoding interface
    bool initialize_decoder(const CodecInfo& codec_info);
    bool decode_packet(const uint8_t* data, size_t size);
    std::shared_ptr<GPUFrame> get_decoded_frame();
    bool flush_decoder();
    
    // Capabilities
    const HWDecoderCaps& get_system_capabilities() const { return system_caps_; }
    std::vector<HWDecodePlatform> get_available_platforms() const;
    
    // Performance monitoring
    struct DecodeStats {
        uint64_t frames_decoded = 0;
        double average_decode_time_ms = 0.0;
        uint64_t hardware_decodes = 0;
        uint64_t software_decodes = 0;
        double hardware_acceleration_ratio = 0.0;
    };
    
    DecodeStats get_decode_statistics() const;
    void reset_statistics();
    
private:
    void detect_available_decoders();
    void initialize_system_capabilities();
    
    std::unique_ptr<PlatformHWDecoder> create_platform_decoder(HWDecodePlatform platform);
    
    void update_decode_stats(bool hardware_decode, double decode_time_ms);
};

} // namespace Shotcut::Video
```

---

## 6.5 VIDEO PREVIEW RENDERING

### 6.5.1 Real-Time Preview System

**Video Preview Philosophy:**
- **GPU-Accelerated Rendering:** Direct Vulkan texture rendering
- **Real-Time Performance:** 60fps preview with effects
- **Multi-Format Support:** Various color spaces and pixel formats
- **Zoom and Pan:** Smooth zoom/pan with level-of-detail
- **Overlay Support:** UI overlays and scope displays

**src/video/video_preview.h:**
```cpp
#pragma once

#include "frame_buffer.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <chrono>

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
    VkDevice vulkan_device_;
    VkQueue vulkan_queue_;
    uint32_t vulkan_queue_family_;
    
    // Rendering pipeline
    VkPipeline preview_pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkRenderPass render_pass_;
    VkDescriptorSetLayout descriptor_set_layout_;
    
    // Framebuffers
    std::vector<VkFramebuffer> framebuffers_;
    VkImage preview_image_;
    VkImageView preview_image_view_;
    VkDeviceMemory preview_image_memory_;
    VkSampler preview_sampler_;
    
    // Preview properties
    uint32_t preview_width_ = 1920;
    uint32_t preview_height_ = 1080;
    ColorSpace color_space_ = ColorSpace::Rec709;
    PreviewQuality quality_ = PreviewQuality::Auto;
    
    // Zoom and pan
    float zoom_level_ = 1.0f;
    glm::vec2 pan_offset_ = glm::vec2(0.0f);
    bool auto_fit_ = false;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    float current_fps_ = 0.0f;
    float target_fps_ = 60.0f;
    uint32_t frame_drops_ = 0;
    uint32_t total_frames_ = 0;
    
    // Uniform buffers
    struct PreviewUniforms {
        glm::mat4 projection_matrix;
        glm::mat4 model_matrix;
        glm::vec4 color_conversion_params; // color space conversion parameters
        glm::vec4 zoom_pan_params;       // zoom and pan parameters
        float time_offset;               // playback time offset
        float quality_factor;             // preview quality factor
        uint32_t flags;                 // rendering flags
    };
    
    std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> uniform_buffers_;
    std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> uniform_buffers_memory_;
    std::array<void*, MAX_FRAMES_IN_FLIGHT> uniform_buffers_mapped_;
    
public:
    VideoPreview(VkDevice device, VkQueue queue, uint32_t queue_family);
    ~VideoPreview();
    
    // Initialization
    bool initialize();
    void cleanup();
    bool is_initialized() const;
    
    // Preview configuration
    void set_resolution(uint32_t width, uint32_t height);
    void set_color_space(ColorSpace color_space);
    void set_quality(PreviewQuality quality);
    void set_target_fps(float fps);
    
    // Zoom and pan
    void set_zoom(float zoom);
    void set_pan(const glm::vec2& offset);
    void set_auto_fit(bool auto_fit);
    void zoom_to_fit();
    void zoom_in(const glm::vec2& center_point);
    void zoom_out(const glm::vec2& center_point);
    
    // Rendering
    bool render_frame(std::shared_ptr<GPUFrame> frame);
    bool render_frame_with_effects(std::shared_ptr<GPUFrame> frame, 
                                const std::vector<std::string>& effect_chain);
    
    // Asynchronous rendering with C++20 coroutines
    std::task<bool> render_frame_async(std::shared_ptr<GPUFrame> frame);
    std::task<bool> render_frame_with_effects_async(
        std::shared_ptr<GPUFrame> frame, 
        const std::vector<std::string>& effect_chain
    );
    
    // Performance
    float get_current_fps() const { return current_fps_; }
    uint32_t get_frame_drops() const { return frame_drops_; }
    float get_frame_drop_rate() const;
    
    void reset_performance_stats();
    
    // Quality management
    void auto_adjust_quality();
    PreviewQuality get_recommended_quality() const;
    
private:
    bool create_render_pass();
    bool create_preview_pipeline();
    bool create_framebuffers();
    bool create_uniform_buffers();
    bool create_preview_image();
    
    VkShaderModule create_vertex_shader();
    VkShaderModule create_fragment_shader();
    
    void update_uniform_buffer(uint32_t current_image, 
                           std::shared_ptr<GPUFrame> frame,
                           const std::vector<std::string>* effects = nullptr);
    
    void calculate_projection_matrix();
    void calculate_color_conversion_params(glm::vec4& params);
    
    void update_performance_stats();
    void adjust_quality_for_performance();
    
    VkFormat get_optimal_format(PixelFormat frame_format);
    bool supports_zero_copy(std::shared_ptr<GPUFrame> frame);
};

// Multi-output preview manager
class PreviewManager {
private:
    struct PreviewOutput {
        std::string name;
        std::unique_ptr<VideoPreview> preview;
        VkImage output_image;
        VkImageView output_image_view;
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchain_images;
        std::vector<VkImageView> swapchain_image_views;
        VkSurfaceKHR surface;
        
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
    bool add_output(const std::string& name, VkSurfaceKHR surface);
    void remove_output(const std::string& name);
    void set_active_output(const std::string& name);
    
    // Preview control
    bool render_to_all_outputs(std::shared_ptr<GPUFrame> frame);
    bool render_to_active_output(std::shared_ptr<GPUFrame> frame);
    
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
    bool create_swapchain_for_output(PreviewOutput& output);
    void recreate_swapchain(PreviewOutput& output);
};

} // namespace Shotcut::Video
```

---

## 6.6 BASIC PLAYBACK CONTROLS

### 6.6.1 Professional Playback System

**Playback Control Philosophy:**
- **Frame-Accurate Timing:** Microsecond-precision playback
- **Smooth Scrubbing:** Frame-by-frame navigation
- **Variable Speed:** 0.1x to 32x playback speeds
- **JKL Navigation:** Professional keyboard navigation
- **Loop Support:** A/B loop points and loop modes

**src/video/playback_controller.h:**
```cpp
#pragma once

#include "video_preview.h"
#include "frame_buffer.h"
#include <chrono>
#include <memory>
#include <functional>
#include <atomic>

namespace Shotcut::Video {

using TimePoint = std::chrono::microseconds;
using Duration = std::chrono::microseconds;

// Playback states
enum class PlaybackState : uint8_t {
    Stopped,
    Playing,
    Paused,
    Seeking,
    Buffering,
    Error
};

// Loop modes
enum class LoopMode : uint8_t {
    None,        // No looping
    Entire,      // Loop entire timeline
    Region,      // Loop A/B region
    Single       // Loop current clip/region
};

// Playback controller with professional features
class PlaybackController {
private:
    std::unique_ptr<VideoPreview> preview_;
    std::unique_ptr<HWDecoderManager> decoder_manager_;
    
    // Timing and synchronization
    TimePoint current_time_{Duration{0}};
    TimePoint start_time_{Duration{0}};
    TimePoint end_time_{Duration{0}};
    TimePoint loop_start_{Duration{0}};
    TimePoint loop_end_{Duration{0}};
    
    // Playback parameters
    float playback_speed_ = 1.0f;
    PlaybackState state_ = PlaybackState::Stopped;
    LoopMode loop_mode_ = LoopMode::None;
    
    // Frame rate control
    AVRational frame_rate_{30, 1};
    TimePoint frame_duration_{Duration{0}};
    TimePoint next_frame_time_{Duration{0}};
    
    // Synchronization
    std::chrono::high_resolution_clock::time_point last_update_;
    std::chrono::microseconds audio_video_offset_{Duration{0}};
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point playback_start_;
    uint64_t frames_played_ = 0;
    uint64_t frames_dropped_ = 0;
    double average_fps_ = 0.0f;
    
    // Callbacks
    std::function<void(TimePoint)> on_time_changed_;
    std::function<void(PlaybackState)> on_state_changed_;
    std::function<void(float)> on_speed_changed_;
    std::function<void()> on_loop_point_reached_;
    std::function<void(const std::string&)> on_error_;
    
public:
    PlaybackController();
    ~PlaybackController();
    
    // Initialization
    bool initialize(std::unique_ptr<VideoPreview> preview);
    void cleanup();
    bool is_initialized() const;
    
    // Media loading
    bool load_media(const std::string& file_path);
    bool load_media_with_decoder(const std::string& file_path, 
                             const std::string& decoder_hint = "auto");
    void unload_media();
    bool is_media_loaded() const;
    
    // Media information
    TimePoint get_duration() const;
    int get_width() const;
    int get_height() const;
    AVRational get_frame_rate() const;
    PixelFormat get_pixel_format() const;
    CodecInfo get_codec_info() const;
    
    // Playback control
    void play();
    void pause();
    void stop();
    void toggle_playback();
    
    // Seeking and navigation
    void seek(TimePoint time);
    void seek_relative(Duration offset);
    void seek_to_frame(int64_t frame_number);
    void seek_to_percentage(float percentage);
    
    // Frame-by-frame navigation
    void step_forward();
    void step_backward();
    void next_frame();
    void previous_frame();
    
    // Speed control
    void set_speed(float speed);
    float get_speed() const { return playback_speed_; }
    void speed_up();
    void speed_down();
    void reset_speed();
    
    // JKL navigation (professional)
    void jkl_navigate(float speed_multiplier); // J: back, K: pause, L: forward
    void jkl_reverse(); // Shift+J for reverse
    void shuttle(float speed); // Variable speed shuttle
    
    // Loop control
    void set_loop_mode(LoopMode mode);
    LoopMode get_loop_mode() const { return loop_mode_; }
    
    void set_loop_points(TimePoint start, TimePoint end);
    void clear_loop_points();
    TimePoint get_loop_start() const { return loop_start_; }
    TimePoint get_loop_end() const { return loop_end_; }
    
    // State and timing
    PlaybackState get_state() const { return state_; }
    TimePoint get_current_time() const { return current_time_; }
    int64_t get_current_frame_number() const;
    float get_playback_percentage() const;
    
    // Synchronization
    void set_audio_video_offset(Duration offset);
    Duration get_audio_video_offset() const { return audio_video_offset_; }
    
    // Callbacks
    void set_time_changed_callback(std::function<void(TimePoint)> callback);
    void set_state_changed_callback(std::function<void(PlaybackState)> callback);
    void set_speed_changed_callback(std::function<void(float)> callback);
    void set_loop_point_callback(std::function<void()> callback);
    void set_error_callback(std::function<void(const std::string&)> callback);
    
    // Performance monitoring
    double get_current_fps() const { return average_fps_; }
    uint64_t get_frames_played() const { return frames_played_; }
    uint64_t get_frames_dropped() const { return frames_dropped_; }
    float get_frame_drop_rate() const;
    
    void reset_performance_stats();
    
    // Professional features
    void enable_frame_dropping(bool enable);
    void set_buffer_size(size_t frames);
    void set_preroll_size(size_t frames);
    
    // Marker support
    void add_marker(TimePoint time, const std::string& name);
    void remove_marker(const std::string& name);
    void seek_to_marker(const std::string& name);
    std::vector<std::pair<TimePoint, std::string>> get_markers() const;
    
private:
    void update_playback_state(PlaybackState new_state);
    void update_current_time(TimePoint new_time);
    
    TimePoint calculate_frame_time(int64_t frame_number) const;
    int64_t calculate_frame_number(TimePoint time) const;
    
    bool should_drop_frame() const;
    bool is_time_in_bounds(TimePoint time) const;
    
    void update_performance_stats();
    void handle_looping();
    
    // Audio synchronization
    void synchronize_audio_video();
    Duration calculate_drift() const;
    void correct_drift(Duration drift);
    
    // Error handling
    void handle_playback_error(const std::string& error);
    void recover_from_error();
};

// Advanced playback features for professional workflows
class AdvancedPlaybackController : public PlaybackController {
private:
    // Multi-angle support
    std::vector<std::string> angle_streams_;
    int current_angle_ = 0;
    
    // Chapter support
    struct Chapter {
        TimePoint start_time;
        TimePoint end_time;
        std::string title;
        std::vector<std::string> tags;
    };
    std::vector<Chapter> chapters_;
    int current_chapter_ = -1;
    
    // Subtitle support
    struct SubtitleTrack {
        std::string language;
        std::string title;
        bool is_default;
        std::vector<std::pair<TimePoint, std::string>> subtitles;
    };
    std::vector<SubtitleTrack> subtitle_tracks_;
    int current_subtitle_track_ = -1;
    
public:
    AdvancedPlaybackController();
    ~AdvancedPlaybackController() override;
    
    // Multi-angle
    std::vector<std::string> get_available_angles() const;
    void set_angle(int angle_index);
    int get_current_angle() const { return current_angle_; }
    
    // Chapter navigation
    std::vector<Chapter> get_chapters() const;
    void next_chapter();
    void previous_chapter();
    void jump_to_chapter(int chapter_index);
    int get_current_chapter() const { return current_chapter_; }
    
    // Subtitle control
    std::vector<SubtitleTrack> get_subtitle_tracks() const;
    void set_subtitle_track(int track_index);
    void toggle_subtitles();
    int get_current_subtitle_track() const { return current_subtitle_track_; }
    
    std::string get_subtitle_at_time(TimePoint time) const;
    
private:
    void load_angle_streams();
    void load_chapters();
    void load_subtitle_tracks();
    
    int find_chapter_at_time(TimePoint time) const;
};

} // namespace Shotcut::Video
```

---

## 6.7 GPU PROCESSING PIPELINE

### 6.7.1 Compute Shader Pipeline

**GPU Processing Philosophy:**
- **Compute Shaders:** Vulkan compute for video effects
- **Zero-Copy Workflows:** GPU-only processing chains
- **Parallel Processing:** Multi-pass parallel computation
- **Memory Efficiency:** Smart GPU memory management
- **Custom Effects:** Extensible effect system

**src/video/gpu_pipeline.h:**
```cpp
#pragma once

#include "frame_buffer.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>

namespace Shotcut::Video {

// Compute shader stages
enum class ComputeStage : uint8_t {
    InputProcessing,    // Input format conversion, debayering
    ColorCorrection,     // Color grading, LUTs
    Effects,           // Blur, sharpen, distortions
    Scaling,           // Resizing, cropping
    OutputProcessing    // Format conversion, encoding prep
};

// GPU effect types
enum class EffectType : uint8_t {
    None,
    Blur,
    Sharpen,
    ColorGrading,
    Crop,
    Scale,
    Rotate,
    Stabilize,
    Denoise,
    Custom
};

// GPU effect parameters (using variant for type safety)
using EffectParameter = std::variant<
    float, int32_t, bool,
    glm::vec2, glm::vec3, glm::vec4,
    std::vector<float>, std::vector<glm::vec4>
>;

struct EffectDefinition {
    std::string name;
    std::string description;
    EffectType type;
    std::vector<std::pair<std::string, EffectParameter>> parameters;
    std::vector<ComputeStage> stages;
    
    // Performance characteristics
    float gpu_time_estimate;      // Estimated GPU time per frame
    float memory_requirement;      // GPU memory requirement in MB
    bool supports_realtime;       // Can run in real-time
    int parallelizable_passes;     // Number of parallelizable passes
};

class GPUComputeEffect {
private:
    std::string effect_name_;
    VkDevice vulkan_device_;
    VkQueue compute_queue_;
    
    // Compute pipeline
    VkPipeline compute_pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    VkDescriptorPool descriptor_pool_;
    VkDescriptorSet descriptor_set_;
    
    // Compute shader
    VkShaderModule compute_shader_;
    std::string shader_source_;
    
    // Buffer management
    struct ComputeBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
        void* mapped_ptr;
        size_t size;
        VkDescriptorBufferInfo descriptor_info;
    };
    
    ComputeBuffer input_buffer_;
    ComputeBuffer output_buffer_;
    ComputeBuffer parameter_buffer_;
    ComputeBuffer temp_buffer_;
    
    // Work group sizing
    uint32_t workgroup_size_x_ = 16;
    uint32_t workgroup_size_y_ = 16;
    uint32_t workgroup_size_z_ = 1;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_execution_;
    float average_execution_time_ms_ = 0.0f;
    uint32_t executions_count_ = 0;
    
public:
    GPUComputeEffect(VkDevice device, VkQueue compute_queue);
    ~GPUComputeEffect();
    
    // Initialization
    bool initialize(const std::string& effect_name, const std::string& shader_code);
    void cleanup();
    bool is_initialized() const;
    
    // Effect execution
    bool apply_effect(std::shared_ptr<GPUFrame> input_frame, 
                    std::shared_ptr<GPUFrame> output_frame,
                    const std::unordered_map<std::string, EffectParameter>& parameters);
    
    // Asynchronous execution with C++20 coroutines
    std::task<bool> apply_effect_async(
        std::shared_ptr<GPUFrame> input_frame,
        std::shared_ptr<GPUFrame> output_frame,
        const std::unordered_map<std::string, EffectParameter>& parameters
    );
    
    // Shader management
    bool reload_shader(const std::string& shader_code);
    bool set_workgroup_size(uint32_t x, uint32_t y, uint32_t z);
    
    // Performance
    float get_average_execution_time() const { return average_execution_time_ms_; }
    uint32_t get_executions_count() const { return executions_count_; }
    void reset_performance_stats();
    
private:
    bool create_compute_pipeline();
    bool create_descriptor_sets();
    bool create_buffers();
    
    VkShaderModule compile_shader(const std::string& source_code);
    void update_parameters(const std::unordered_map<std::string, EffectParameter>& parameters);
    
    void dispatch_compute(uint32_t width, uint32_t height, uint32_t depth = 1);
    void update_performance_stats(std::chrono::nanoseconds execution_time);
};

// Multi-pass GPU processing pipeline
class GPUPipeline {
private:
    struct PipelineStage {
        std::unique_ptr<GPUComputeEffect> effect;
        std::shared_ptr<GPUFrame> input_frame;
        std::shared_ptr<GPUFrame> output_frame;
        std::unordered_map<std::string, EffectParameter> parameters;
        bool is_enabled;
    };
    
    std::vector<PipelineStage> stages_;
    std::vector<std::shared_ptr<GPUFrame>> intermediate_frames_;
    VkDevice vulkan_device_;
    
    // Pipeline synchronization
    std::vector<VkSemaphore> stage_semaphores_;
    std::vector<VkFence> stage_fences_;
    uint32_t current_frame_ = 0;
    
    // Performance optimization
    bool pipeline_optimization_enabled_ = true;
    std::unordered_map<std::string, float> effect_times_;
    
public:
    GPUPipeline(VkDevice device);
    ~GPUPipeline();
    
    // Pipeline construction
    bool add_stage(const std::string& effect_name, 
                  const std::string& shader_code,
                  const std::unordered_map<std::string, EffectParameter>& parameters = {});
    
    bool remove_stage(const std::string& effect_name);
    void clear_stages();
    
    // Pipeline execution
    bool execute_pipeline(std::shared_ptr<GPUFrame> input_frame, 
                       std::shared_ptr<GPUFrame> output_frame);
    
    // Asynchronous pipeline execution
    std::task<std::shared_ptr<GPUFrame>> execute_pipeline_async(
        std::shared_ptr<GPUFrame> input_frame
    );
    
    // Stage management
    void enable_stage(const std::string& effect_name);
    void disable_stage(const std::string& effect_name);
    bool is_stage_enabled(const std::string& effect_name) const;
    
    void update_stage_parameters(const std::string& effect_name, 
                            const std::unordered_map<std::string, EffectParameter>& parameters);
    
    // Performance optimization
    void enable_pipeline_optimization(bool enable);
    bool optimize_pipeline_order();
    void merge_compatible_stages();
    
    // Performance analysis
    struct PipelineProfile {
        float total_time_ms;
        std::vector<std::pair<std::string, float>> stage_times;
        std::vector<std::string> bottlenecks;
        float memory_usage_mb;
        float gpu_utilization;
    };
    
    PipelineProfile profile_pipeline() const;
    std::vector<std::string> get_optimization_suggestions() const;
    
private:
    bool create_synchronization_objects();
    void cleanup_synchronization_objects();
    
    bool optimize_frame_allocation();
    bool optimize_effect_order();
    
    void record_stage_commands(size_t stage_index, VkCommandBuffer command_buffer);
    void submit_stage_commands(size_t stage_index, VkCommandBuffer command_buffer);
    
    void wait_for_stage_completion(size_t stage_index);
    void update_stage_performance(const std::string& effect_name, float time_ms);
};

// Built-in effect library
class EffectLibrary {
private:
    static std::unique_ptr<EffectLibrary> instance_;
    std::unordered_map<std::string, EffectDefinition> effects_;
    
public:
    static EffectLibrary& get() {
        if (!instance_) {
            instance_ = std::make_unique<EffectLibrary>();
        }
        return *instance_;
    }
    
    bool initialize();
    void cleanup();
    
    // Effect access
    const EffectDefinition* get_effect(const std::string& name) const;
    std::vector<std::string> get_available_effects() const;
    std::vector<std::string> get_effects_by_type(EffectType type) const;
    
    // Shader source access
    std::string get_effect_shader(const std::string& name) const;
    std::string get_effect_shader_template(const std::string& name) const;
    
    // Custom effect registration
    bool register_custom_effect(const EffectDefinition& effect);
    bool unregister_effect(const std::string& name);
    
private:
    void load_builtin_effects();
    void load_custom_effects();
    
    void register_blur_effects();
    void register_sharpen_effects();
    void register_color_correction_effects();
    void register_geometry_effects();
    
    std::string load_shader_from_file(const std::string& file_path);
    bool validate_shader(const std::string& shader_code);
};

} // namespace Shotcut::Video
```

---

## PHASE 6 IMPLEMENTATION ROADMAP

### 6.8 Month-by-Month Implementation Plan

**Month 7: Core Video Processing**
- Week 1-2: FFmpeg integration with security hardening
- Week 3-4: NVIDIA Video Codec SDK integration
- Week 5-6: Frame buffer management system
- Week 7-8: Hardware-accelerated decoding implementation

**Month 8: Preview and Rendering**
- Week 1-2: Video preview rendering system
- Week 3-4: Real-time playback controls
- Week 5-6: Zoom, pan, and navigation
- Week 7-8: Multi-display and monitor support

**Month 9: GPU Processing Pipeline**
- Week 1-2: Compute shader framework
- Week 3-4: Built-in effect library
- Week 5-6: Multi-pass pipeline optimization
- Week 7-8: Custom effect system

**Month 10: Performance and Integration**
- Week 1-2: Performance optimization and profiling
- Week 3-4: Memory usage optimization
- Week 5-6: Error handling and recovery
- Week 7-8: Integration testing and validation

### 6.9 Critical Success Metrics

**Performance Targets:**
- Decode performance: 4K H.264 at 60fps on RTX 3060+
- Encode performance: 4K H.265 at 30fps on RTX 3060+
- Preview rendering: <8ms frame time for 1080p
- GPU utilization: <80% for normal workflows
- Memory efficiency: <2GB for 1080p workflows

**Quality Targets:**
- Zero frame drops in normal playback
- <1 frame sync drift over 10 minutes
- Pixel-perfect color space conversion
- Professional-level color grading accuracy

**Professional Features:**
- Real-time effects preview at 60fps
- Support for 4K, 8K, and HDR content
- Professional color space support (Rec.709, Rec.2020)
- Hardware acceleration for all major codecs
- Zero-copy workflows where possible

This Phase 6 implementation guide provides the foundation for building a professional-grade video processing pipeline that rivals commercial video editors, with modern C++20/23 architecture, comprehensive hardware acceleration, and real-time performance capabilities.
