#pragma once

// NVIDIA Video Codec SDK Integration
// Real implementation for hardware-accelerated video processing
// Conditional compilation for graceful degradation when SDK is not available

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <chrono>
#include <optional>
#include <mutex>
#include <queue>

// Conditional SDK includes
#ifdef ENABLE_NVIDIA_SDK
#include <cuda_runtime.h>
#include <nvEncodeAPI.h>
#include <nvcuvid.h>
#endif

#ifdef ENABLE_VULKAN
#include <vulkan/vulkan.h>
#else
// Define minimal Vulkan types for compilation
struct VkDevice_T { void* handle; };
using VkDevice = VkDevice_T*;
struct VkImage_T { void* handle; };
using VkImage = VkImage_T*;
struct VkImageView_T { void* handle; };
using VkImageView = VkImageView_T*;
struct VkQueue_T { void* handle; };
using VkQueue = VkQueue_T*;
const uint32_t VK_NULL_HANDLE = 0;
#endif

// Define NVIDIA SDK types when SDK is not available
#ifndef ENABLE_NVIDIA_SDK

// Forward declarations for NVIDIA SDK types
typedef void* CUcontext;
typedef void* CUdevice;
typedef unsigned long long CUdeviceptr;
typedef void* CUvideodecoder;
typedef void* CUvideoparser;
typedef void* CUgraphicsResource;
typedef void* NV_ENC_OUTPUT_PTR;
typedef void* NV_ENC_REGISTERED_PTR;

// Enum definitions
enum NV_ENC_BUFFER_FORMAT {
    NV_ENC_BUFFER_FORMAT_YUV420 = 0,
    NV_ENC_BUFFER_FORMAT_YUV444 = 1,
    NV_ENC_BUFFER_FORMAT_ARGB = 2,
    NV_ENC_BUFFER_FORMAT_ABGR = 3
};

enum NV_ENC_OUTPUT_FORMAT {
    NV_ENC_OUTPUT_FORMAT_H264 = 0,
    NV_ENC_OUTPUT_FORMAT_HEVC = 1,
    NV_ENC_OUTPUT_FORMAT_AV1 = 2,
    NV_ENC_OUTPUT_FORMAT_YUV420 = 3
};

enum NV_ENC_PARAMS_RC_MODE {
    NV_ENC_PARAMS_RC_VBR = 0,
    NV_ENC_PARAMS_RC_CBR = 1,
    NV_ENC_PARAMS_RC_CONSTQP = 2
};

enum NV_CODEC_TYPE {
    NV_CODEC_TYPE_NONE = 0,
    NV_CODEC_TYPE_H264 = 1,
    NV_CODEC_TYPE_HEVC = 2,
    NV_CODEC_TYPE_AV1 = 3
};

typedef void* NV_ENC_PRESET;
#define NV_ENC_PRESET_DEFAULT_GUID ((NV_ENC_PRESET)0)
#define NV_ENC_CODEC_H264 ((void*)1)
#define NV_ENC_CODEC_HEVC ((void*)2)
#define NV_ENC_CODEC_AV1 ((void*)3)
#define NV_ENC_CODEC void*

struct NV_ENC_CAPS {
    uint32_t supported;
    uint32_t max_width;
    uint32_t max_height;
    uint32_t max_bitrate;
    uint32_t async_encoding_support;
};

struct NV_ENCODE_API_FUNCTION_LIST {
    void* nvEncOpenEncodeSessionEx;
    void* nvEncGetEncodeGUIDs;
    void* nvEncGetEncodeProfileGUIDs;
    void* nvEncGetEncodePresetGUIDs;
    void* nvEncGetInputFormats;
    void* nvEncGetEncodeCaps;
    void* nvEncGetEncodeStat;
    void* nvEncGetSequenceParams;
    void* nvEncRegisterResource;
    void* nvEncUnregisterResource;
    void* nvEncMapResource;
    void* nvEncUnmapResource;
    void* nvEncLockInputBuffer;
    void* nvEncUnlockInputBuffer;
    void* nvEncEncodePicture;
    void* nvEncInvalidateRefFrames;
    void* nvEncLockBitstream;
    void* nvEncUnlockBitstream;
    void* nvEncGetLastError;
};

struct NV_ENC_INITIALIZE_PARAMS {
    uint32_t version;
    void* encodeGUID;
    void* presetGUID;
    uint32_t encodeWidth;
    uint32_t encodeHeight;
    uint32_t maxEncodeWidth;
    uint32_t maxEncodeHeight;
    uint32_t darWidth;
    uint32_t darHeight;
    uint32_t frameRateNum;
    uint32_t frameRateDen;
    uint32_t enableEncodeAsync;
    uint32_t enablePTD;
    uint32_t reportSliceOffsets;
    uint32_t enableSubFrameWrite;
    uint32_t enableExternalMEHints;
    uint32_t enableMEOnlyMode;
    uint32_t enableWeightedPrediction;
    uint32_t mode;
    uint32_t reserved1;
    uint64_t reserved2[239];
    void* privData;
    void* reserved3[64];
};

struct CUVIDEOFORMAT {
    uint32_t codec;
    uint32_t frame_rate;
    uint32_t progressive_sequence;
    uint32_t progressive_frame;
    uint32_t coded_width;
    uint32_t coded_height;
    uint32_t display_area;
    uint32_t video_signal_description;
    uint32_t bitdepth_luma_minus8;
    uint32_t bitdepth_chroma_minus8;
    uint32_t min_num_decode_surfaces;
    uint32_t max_num_decode_surfaces;
    uint32_t frame_rate_description;
    uint32_t video_format;
    uint32_t color_primaries;
    uint32_t transfer_characteristics;
    uint32_t matrix_coefficients;
    uint32_t sequence_ext_param;
    uint32_t sequence_ext_data;
    uint32_t vui_param;
    uint32_t vui_data;
    uint32_t seq_hdr_data_length;
    uint32_t num_seq_ext_hdr;
    uint32_t pic_ext_param;
    uint32_t pic_ext_data;
    uint32_t pic_hdr_data_length;
    uint32_t num_pic_ext_hdr;
    uint32_t chroma_format;
    uint32_t nal_unit_length;
    uint32_t luma_log2_denominator;
    uint32_t reserved[246];
};

struct CUVIDPARSERDISPINFO {
    int picture_index;
    int progressive_frame;
    int top_field_first;
    int repeat_first_field;
    uint64_t timestamp;
    uint32_t display_order;
    uint32_t reserved[6];
};

struct CUVIDPICPARAMS {
    uint32_t PicWidthInMbs;
    uint32_t FrameHeightInMbs;
    uint32_t CurrPicIdx;
    uint32_t field_pic_flag;
    uint32_t bottom_field_flag;
    uint32_t second_field;
    uint32_t picture_index;
    uint32_t progressive_frame;
    uint32_t num_ref_frames;
    uint32_t CodecSpecificParamCount;
    uint32_t reserved[8];
    uint64_t CodecSpecificParams[16];
    uint64_t reserved2[8];
};

struct CUVIDDECODECREATEINFO {
    uint32_t ulWidth;
    uint32_t ulHeight;
    uint32_t ulNumDecodeSurfaces;
    uint32_t CodecType;
    uint32_t ulCreationFlags;
    uint32_t bitDepthMinus8;
    uint32_t DeinterlaceMode;
    uint32_t ulTargetWidth;
    uint32_t ulTargetHeight;
    uint32_t ulNumOutputSurfaces;
    uint32_t ulIntraDecodeThresh;
    uint32_t reserved[16];
};

#endif // !ENABLE_NVIDIA_SDK

namespace Shotcut::Video {

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
    
    // Frame rate
    uint32_t frame_rate_num = 30;
    uint32_t frame_rate_den = 1;
    
    // Quality preset
    NV_ENC_PRESET preset = NV_ENC_PRESET_DEFAULT_GUID;
    
    static NVENCConfig create_default() { return {}; }
    static NVENCConfig create_high_quality();
    static NVENCConfig create_low_latency();
};

class NVENCEncoder {
private:
    NV_ENCODE_API_FUNCTION_LIST* nvenc_api_ = nullptr;
    void* encoder_interface_ = nullptr;
    NV_ENC_INITIALIZE_PARAMS init_params_{};
    
    // CUDA context
    CUcontext cuda_context_ = nullptr;
    CUdevice cuda_device_ = 0;
    int cuda_device_id_ = 0;
    
    // Buffer management
    struct InputBuffer {
        CUdeviceptr device_ptr = 0;
        void* host_ptr = nullptr;
        size_t size = 0;
        NV_ENC_REGISTERED_PTR registered_ptr = nullptr;
        bool is_mapped = false;
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
    
    bool initialized_ = false;
    
public:
    NVENCEncoder();
    ~NVENCEncoder();
    
    // Initialization
    bool initialize(const NVENCConfig& config, int cuda_device_id = 0);
    void cleanup();
    bool is_initialized() const { return initialized_; }
    
    // Encoding
    bool encode_frame(const uint8_t* yuv_data, size_t frame_size);
    bool flush_encoder();
    std::vector<uint8_t> get_encoded_data();
    
    // Configuration
    void update_bitrate(uint32_t target_bitrate, uint32_t max_bitrate = 0);
    void update_frame_rate(uint32_t fps_num, uint32_t fps_den);
    void update_resolution(int width, int height);
    
    // Status and statistics
    uint64_t get_frames_encoded() const { return frames_encoded_; }
    double get_average_encode_time() const { return average_encode_time_; }
    uint32_t get_current_fps() const { return current_fps_; }
    
    // Hardware capabilities
    static bool is_available();
    static std::vector<int> get_available_cuda_devices();
    static NVENC_CAPS get_encoder_caps(void* codec);
    
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
    
    static void HandleBitstream(void* user_data, NV_ENC_OUTPUT_PTR bitstream_ptr);
    static void HandleCompletionEvent(void* user_data, NV_ENC_OUTPUT_PTR completion_event);
};

// NVDEC decoder for hardware decoding
class NVDECDecoder {
private:
    CUvideodecoder video_decoder_ = nullptr;
    CUvideoparser video_parser_ = nullptr;
    CUcontext cuda_context_ = nullptr;
    CUdevice cuda_device_ = 0;
    
    // Frame management
    struct DecodedFrame {
        CUdeviceptr device_ptr = 0;
        CUgraphicsResource vulkan_resource = nullptr;
        VkImage vulkan_image = VK_NULL_HANDLE;
        VkImageView vulkan_image_view = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
        NV_ENC_OUTPUT_PTR bitstream_ptr = nullptr;
        bool is_available = false;
        bool is_mapped = false;
    };
    
    std::vector<std::unique_ptr<DecodedFrame>> frame_pool_;
    std::queue<size_t> available_frame_indices_;
    std::mutex frame_mutex_;
    
    // Vulkan interop
    VkDevice vulkan_device_ = VK_NULL_HANDLE;
    VkQueue vulkan_queue_ = VK_NULL_HANDLE;
    uint32_t vulkan_queue_family_ = 0;
    
    // Codec information
    NV_CODEC_TYPE codec_type_ = NV_CODEC_TYPE_NONE;
    int width_ = 0;
    int height_ = 0;
    NV_ENC_OUTPUT_FORMAT output_format_ = NV_ENC_OUTPUT_FORMAT_YUV420;
    
    // Performance tracking
    std::chrono::high_resolution_clock::time_point start_time_;
    uint64_t frames_decoded_ = 0;
    double average_decode_time_ = 0.0;
    
    bool initialized_ = false;
    
public:
    NVDECDecoder();
    ~NVDECDecoder();
    
    // Initialization
    bool initialize(const std::string& codec_name, VkDevice vulkan_device = VK_NULL_HANDLE);
    void cleanup();
    bool is_initialized() const { return initialized_; }
    
    // Decoding
    bool decode_packet(const uint8_t* packet_data, size_t packet_size);
    std::optional<VkImage> get_decoded_frame();
    bool flush_decoder();
    
    // Configuration
    void set_resolution(int width, int height);
    void set_output_format(NV_ENC_OUTPUT_FORMAT format);
    
    // Status
    uint32_t get_frame_count() const;
    uint64_t get_frames_decoded() const { return frames_decoded_; }
    double get_average_decode_time() const { return average_decode_time_; }
    
    // Hardware capabilities
    static bool is_available();
    static bool supports_codec(const std::string& codec_name);
    
private:
    bool setup_cuda_context();
    bool create_video_decoder(const std::string& codec_name);
    bool setup_vulkan_interop(VkDevice vulkan_device);
    
    CUVIDDECODECREATEINFO get_decoder_create_info() const;
    
    bool allocate_frame_pool(size_t frame_count = 16);
    std::unique_ptr<DecodedFrame> get_available_frame();
    
    static int HandleVideoSequence(void* user_data, CUVIDEOFORMAT* format);
    static int HandlePictureDisplay(void* user_data, CUVIDPARSERDISPINFO* disp_info);
    static int HandlePictureDecode(void* user_data, CUVIDPICPARAMS* pic_params);
    
    void update_performance_stats(std::chrono::nanoseconds decode_time);
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
    
    bool initialized_ = false;
    
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
    std::unique_ptr<NVDECDecoder> create_decoder(const std::string& codec_name, VkDevice vulkan_device = VK_NULL_HANDLE);
    
    bool is_initialized() const { return initialized_; }
    
private:
    void detect_nvenc_availability();
    void detect_nvdec_availability();
    void enumerate_cuda_devices();
    void query_driver_versions();
    
    bool load_nvenc_library();
    bool test_nvenc_functionality();
    bool test_nvdec_functionality();
};

} // namespace Shotcut::Video
