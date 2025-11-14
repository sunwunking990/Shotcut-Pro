#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <string>
#include <functional>

namespace Shotcut::Video {

// Pixel format enumeration for video frames
enum class PixelFormat {
    UNKNOWN = 0,
    RGB8,
    RGBA8,
    RGB16,
    RGBA16,
    YUV420P,
    YUV422P,
    YUV444P,
    NV12,
    NV21,
    BGRA8,
    BGR8,
    YUYV422,
    UYVY422,
    GRAY8,
    GRAY16
};

// Frame type enumeration
enum class FrameType {
    UNKNOWN = 0,
    I_FRAME,    // Key frame
    P_FRAME,    // Predictive frame
    B_FRAME,    // Bidirectional frame
};


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
    
    // CUDA interop (placeholder for now)
    void* cuda_device_ptr_;
    void* mapped_cuda_ptr_;
    
    // Frame properties
    FrameDescriptor descriptor_;
    bool is_mapped_ = false;
    bool is_in_use_ = false;
    std::chrono::high_resolution_clock::time_point last_access_time_;
    
    // Frame metadata
    int64_t timestamp = 0;
    FrameType frame_type = FrameType::UNKNOWN;
    
    // Reference counting
    std::atomic<uint32_t> ref_count_;
    
    // Device reference
    VkDevice device_;
    
public:
    GPUFrame(VkDevice device, const FrameDescriptor& desc);
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
    
    // CUDA access (placeholder)
    void* get_cuda_device_ptr() const { return cuda_device_ptr_; }
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
    
    // Data access
    std::vector<uint8_t> get_cpu_data() const;
    
    // Validity
    bool is_valid() const;
    
private:
    bool create_vulkan_resources();
    void cleanup_vulkan_resources();
    
    VkFormat pixel_format_to_vulkan(PixelFormat format);
};

// Frame pool with smart caching
class FramePool {
private:
    struct PoolBucket {
        FrameDescriptor descriptor;
        std::queue<std::shared_ptr<GPUFrame>> available_frames;
        std::vector<std::shared_ptr<GPUFrame>> all_frames;
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
    
    // Cleanup thread
    std::thread cleanup_thread_;
    std::atomic<bool> cleanup_running_;
    std::chrono::seconds cleanup_interval_{std::chrono::seconds(5)};
    
public:
    FramePool(VkDevice vulkan_device, 
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
    struct PoolStatistics {
        size_t total_allocated;
        size_t peak_usage;
        size_t cache_hits;
        size_t cache_misses;
        double hit_rate;
        size_t available_frames;
        size_t in_use_frames;
    };
    
    PoolStatistics get_pool_statistics(const FrameDescriptor& descriptor) const;
    std::vector<PoolStatistics> get_all_statistics() const;
    
    void reset_statistics();
    
    // Memory pressure handling
    void set_memory_pressure_callback(std::function<void(float)> callback);
    void trigger_cleanup();
    
private:
    PoolBucket* get_or_create_bucket(const FrameDescriptor& descriptor);
    std::shared_ptr<GPUFrame> create_new_frame(const FrameDescriptor& descriptor);
    
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

// Hash specialization for FrameDescriptor
namespace std {
    template<>
    struct hash<Shotcut::Video::FrameDescriptor> {
        size_t operator()(const Shotcut::Video::FrameDescriptor& desc) const noexcept {
            return desc.hash();
        }
    };
}
