# Phase 8: Effects & Processing Implementation Guide - Shotcut C++ Transformation

**Document Created:** November13, 2025  
**Implementation Focus:** Professional GPU Effects Processing System  
**Duration:** Months 15-18 (Effects & Processing Phase)  
**Prerequisites:** Phase 7 completed (Timeline & Editing Features)

---

## PHASE 8 OVERVIEW

Phase 8 focuses on creating a professional-grade effects and processing system that rivals commercial editors like Adobe Premiere Pro and DaVinci Resolve. This system will implement GPU-accelerated effects processing using Vulkan compute shaders, real-time preview capabilities, and comprehensive effect management with professional-grade quality and performance.

**Critical Success Factors:**
- **GPU-Accelerated Processing:** Vulkan compute shaders for all effects
- **Real-Time Preview:** 60fps effects preview even with 4K footage
- **Professional Quality:** Broadcast-quality effects and color grading
- **Modular Architecture:** Extensible effect system with plugin support
- **Memory Efficiency:** Smart GPU memory management for complex effect chains

---

## 8.1 EFFECTS ARCHITECTURE DESIGN

### 8.1.1 GPU Effects Processing Framework

**Effects Architecture Philosophy:**
- **Compute-First Design:** All effects run as GPU compute shaders
- **Shader Modularization:** Reusable shader components and utilities
- **Effect Chaining:** Sophisticated effect composition and blending
- **Performance Optimization:** GPU memory pooling and async processing
- **Professional Quality:** 32-bit floating-point precision throughout

**src/effects/effects_framework.h:**
```cpp
#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace Shotcut::Effects {

// Effect processing types
enum class EffectType : uint8_t {
    None = 0,
    
    // Color Correction
    ColorCorrection = 1,
    ColorGrading,
    LiftGammaGain,
    Curves,
    ColorWheel,
    VectorScope,
    
    // Color Adjustment
    BrightnessContrast = 10,
    Saturation,
    Hue,
    Temperature,
    Tint,
    
    // Image Adjustment
    Blur = 20,
    Sharpen,
    UnsharpMask,
    GaussianBlur,
    BoxBlur,
    MotionBlur,
    RadialBlur,
    
    // Geometric Effects
    Crop = 30,
    Scale,
    Rotate,
    Perspective,
    Warp,
    Distort,
    Ripple,
    Waves,
    
    // Composite Effects
    Composite = 40,
    Blend,
    Overlay,
    Screen,
    Multiply,
    Matte,
    Key,
    
    // Noise and Grain
    Noise = 50,
    Grain,
    FilmGrain,
    DigitalNoise,
    
    // Time-Based Effects
    Transition = 60,
    Stabilization,
    FrameInterpolation,
    
    // Audio Effects
    AudioEffect = 70,
    Equalizer,
    Compressor,
    Limiter,
    Reverb,
    
    // Custom Effects
    Custom = 100
};

// Effect processing stages
enum class ProcessingStage : uint8_t {
    Input = 0,        // Input preparation and validation
    Process = 1,      // Main effect processing
    Output = 2         // Output preparation and composition
};

// Effect precision modes
enum class EffectPrecision : uint8_t {
    Half = 16,        // 16-bit floating point
    Single = 32,       // 32-bit floating point (default)
    Double = 64        // 64-bit floating point
};

// GPU compute shader descriptor
struct ComputeShaderDescriptor {
    VkShaderModule shader_module;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout descriptor_set_layout;
    
    std::string entry_point;
    std::string shader_name;
    
    // Work group dimensions
    uint32_t workgroup_size_x = 16;
    uint32_t workgroup_size_y = 16;
    uint32_t workgroup_size_z = 1;
    
    // Shader resources
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkPushConstantRange> push_constants;
    
    // Performance characteristics
    uint32_t local_memory_size = 0;
    uint32_t shared_memory_size = 0;
    bool supports_wavefront = false;
};

// GPU buffer descriptor
struct GPUBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void* mapped_ptr;
    VkDescriptorBufferInfo descriptor_info;
    
    size_t size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_properties;
    
    // Buffer state
    bool is_mapped = false;
    bool needs_update = true;
    uint32_t update_version = 0;
};

// Effect base class
class Effect {
private:
    std::string id_;
    std::string name_;
    std::string category_;
    std::string description_;
    EffectType type_;
    
    // Shader and pipeline
    ComputeShaderDescriptor compute_shader_;
    
    // Input/Output buffers
    GPUBuffer input_buffer_;
    GPUBuffer output_buffer_;
    GPUBuffer temp_buffer_;
    
    // Parameters
    std::unordered_map<std::string, std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>> parameters_;
    
    // Effect state
    bool is_enabled_ = true;
    bool needs_update_ = true;
    EffectPrecision precision_ = EffectPrecision::Single;
    
    // Performance metrics
    std::chrono::high_resolution_clock::time_point last_execution_;
    float average_execution_time_ms_ = 0.0f;
    uint32_t execution_count_ = 0;
    
public:
    Effect(const std::string& id, const std::string& name, EffectType type);
    virtual ~Effect() = default;
    
    // Basic properties
    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& category() const { return category_; }
    const std::string& description() const { return description_; }
    EffectType get_type() const { return type_; }
    
    // Effect management
    virtual bool initialize(VkDevice device, VkQueue compute_queue) = 0;
    virtual void cleanup() = 0;
    
    virtual void set_enabled(bool enabled);
    virtual bool is_enabled() const { return is_enabled_; }
    
    // Parameter management
    virtual void set_parameter(const std::string& name, const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value);
    virtual std::optional<std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>> get_parameter(const std::string& name) const;
    
    virtual std::vector<std::string> get_parameter_names() const;
    virtual std::vector<std::pair<std::string, std::string>> get_parameter_info() const;
    
    // Processing
    virtual bool process(const GPUBuffer& input, GPUBuffer& output, 
                        const glm::ivec2& size, ProcessingStage stage) = 0;
    
    // Shader management
    virtual bool load_shader(const std::string& shader_source) = 0;
    virtual bool compile_shader() = 0;
    virtual bool create_compute_pipeline(VkDevice device) = 0;
    
    // Performance monitoring
    virtual float get_average_execution_time() const { return average_execution_time_ms_; }
    virtual uint32_t get_execution_count() const { return execution_count_; }
    virtual void reset_performance_metrics();
    
    // Validation
    virtual bool validate_parameters() const = 0;
    virtual std::vector<std::string> get_validation_errors() const = 0;
    
protected:
    // Helper functions for subclasses
    VkDevice get_device() const { return device_; }
    VkQueue get_compute_queue() const { return compute_queue_; }
    
    bool create_buffer(size_t size, VkBufferUsageFlags usage, GPUBuffer& buffer);
    void destroy_buffer(GPUBuffer& buffer);
    
    bool map_buffer(GPUBuffer& buffer);
    void unmap_buffer(GPUBuffer& buffer);
    
    void update_performance_metrics(std::chrono::nanoseconds execution_time);
    
    // Parameter helpers
    float get_float_parameter(const std::string& name, float default_value = 0.0f) const;
    int get_int_parameter(const std::string& name, int default_value = 0) const;
    bool get_bool_parameter(const std::string& name, bool default_value = false) const;
    glm::vec2 get_vec2_parameter(const std::string& name, const glm::vec2& default_value = glm::vec2(0.0f)) const;
    glm::vec3 get_vec3_parameter(const std::string& name, const glm::vec3& default_value = glm::vec3(0.0f)) const;
    glm::vec4 get_vec4_parameter(const std::string& name, const glm::vec4& default_value = glm::vec4(0.0f)) const;
    
private:
    VkDevice device_;
    VkQueue compute_queue_;
    
    std::vector<ComputeShaderDescriptor> all_shaders_;
    static constexpr size_t MAX_SHADER_CACHE_SIZE = 100;
};

// Effects manager
class EffectsManager {
private:
    std::unordered_map<std::string, std::unique_ptr<Effect>> effects_;
    std::unordered_map<EffectType, std::vector<std::string>> effects_by_type_;
    
    // GPU resources
    VkDevice device_;
    VkQueue compute_queue_;
    uint32_t compute_queue_family_;
    
    // Command pool and buffers
    VkCommandPool command_pool_;
    std::vector<VkCommandBuffer> command_buffers_;
    uint32_t current_command_buffer_ = 0;
    
    // Memory management
    struct GPUBufferPool {
        std::vector<GPUBuffer> available_buffers;
        std::vector<GPUBuffer> used_buffers;
        size_t total_size = 0;
        size_t max_size = 256 * 1024 * 1024; // 256MB
    };
    
    GPUBufferPool buffer_pool_;
    
    // Shader compilation
    struct ShaderCompilationCache {
        std::string source_code;
        VkShaderModule module;
        std::string compilation_log;
        bool is_compiled = false;
    };
    
    std::unordered_map<std::string, ShaderCompilationCache> shader_cache_;
    
    // Performance monitoring
    struct PerformanceMetrics {
        std::chrono::high_resolution_clock::time_point last_reset;
        uint32_t total_effects_processed = 0;
        float total_processing_time_ms = 0.0f;
        std::unordered_map<std::string, float> effect_times;
        std::unordered_map<EffectType, float> category_times;
    };
    
    PerformanceMetrics performance_metrics_;
    
    // Background processing
    std::thread processing_thread_;
    std::atomic<bool> processing_running_{false};
    std::queue<std::function<void()>> processing_queue_;
    
    // Effect chains
    struct EffectChain {
        std::string id;
        std::vector<std::string> effect_ids;
        std::vector<std::string> effect_parameters;
        bool is_enabled = true;
    };
    
    std::unordered_map<std::string, EffectChain> effect_chains_;
    
public:
    EffectsManager(VkDevice device, VkQueue compute_queue, uint32_t compute_queue_family);
    ~EffectsManager();
    
    // Effect registration
    bool register_effect(std::unique_ptr<Effect> effect);
    void unregister_effect(const std::string& effect_id);
    
    Effect* get_effect(const std::string& effect_id) const;
    std::vector<Effect*> get_effects_by_type(EffectType type) const;
    std::vector<std::string> get_all_effect_ids() const;
    
    // Effect chains
    std::string create_effect_chain(const std::vector<std::string>& effect_ids,
                                 const std::string& name = "");
    void add_effect_to_chain(const std::string& chain_id, const std::string& effect_id);
    void remove_effect_from_chain(const std::string& chain_id, const std::string& effect_id);
    
    bool set_chain_enabled(const std::string& chain_id, bool enabled);
    void set_chain_parameter(const std::string& chain_id, const std::string& effect_id,
                         const std::string& parameter, 
                         const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value);
    
    // GPU processing
    bool process_effect_chain(const std::string& chain_id, const GPUBuffer& input,
                          GPUBuffer& output, const glm::ivec2& size);
    
    bool process_single_effect(const std::string& effect_id, const GPUBuffer& input,
                            GPUBuffer& output, const glm::ivec2& size);
    
    // Asynchronous processing
    std::future<bool> process_effect_chain_async(const std::string& chain_id, const GPUBuffer& input,
                                               GPUBuffer& output, const glm::ivec2& size);
    
    // Resource management
    GPUBuffer allocate_buffer(size_t size, VkBufferUsageFlags usage);
    void deallocate_buffer(const GPUBuffer& buffer);
    
    VkCommandBuffer begin_command_buffer();
    void end_command_buffer(VkCommandBuffer command_buffer);
    void submit_command_buffer(VkCommandBuffer command_buffer);
    
    // Performance monitoring
    const PerformanceMetrics& get_performance_metrics() const { return performance_metrics_; }
    void reset_performance_metrics();
    
    std::vector<std::pair<std::string, float>> get_effect_performance_ranking() const;
    
    // Validation
    bool validate_all_effects() const;
    std::vector<std::string> get_all_validation_errors() const;
    
private:
    bool initialize_gpu_resources();
    void cleanup_gpu_resources();
    
    void start_background_processing();
    void stop_background_processing();
    void processing_thread_function();
    
    void process_background_queue();
    void update_performance_metrics(const std::string& effect_id, float execution_time);
    
    bool compile_shader_cached(const std::string& source_code, VkShaderModule& module);
    void cleanup_shader_cache();
};

} // namespace Shotcut::Effects
```

### 8.1.2 Shader Library and Utilities

**Shader Library Philosophy:**
- **Reusable Components:** Common shader functions and utilities
- **Cross-Platform Compatibility:** SPIR-V shaders with fallback GLSL
- **Performance Optimization:** Shader specialization and optimization
- **Professional Quality:** Broadcast-quality image processing algorithms

**src/effects/shader_library.h:**
```cpp
#pragma once

#include "effects_framework.h"
#include <glm/glm.hpp>
#include <array>

namespace Shotcut::Effects {

// Common shader constants
namespace ShaderConstants {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float HALF_PI = PI * 0.5f;
    
    constexpr float EPSILON = 1e-6f;
    constexpr float FLT_MAX = std::numeric_limits<float>::max();
    constexpr float FLT_MIN = std::numeric_limits<float>::min();
    
    // Color space constants
    constexpr glm::vec3 REC709_R = glm::vec3(0.2126f, 0.7152f, 0.0722f);
    constexpr glm::vec3 REC709_G = glm::vec3(0.2126f, 0.7152f, 0.0722f);
    constexpr glm::vec3 REC709_B = glm::vec3(0.2126f, 0.7152f, 0.0722f);
    
    constexpr glm::vec3 REC2020_R = glm::vec3(0.2627f, 0.6780f, 0.0593f);
    constexpr glm::vec3 REC2020_G = glm::vec3(0.2627f, 0.6780f, 0.0593f);
    constexpr glm::vec3 REC2020_B = glm::vec3(0.2627f, 0.6780f, 0.0593f);
    
    // Gamma correction constants
    constexpr float GAMMA_ENCODE = 2.2f;
    constexpr float GAMMA_DECODE = 1.0f / 2.2f;
}

// Color space conversion utilities
struct ColorSpaceUtils {
    // sRGB to linear
    static float srgb_to_linear(float srgb) {
        return srgb <= 0.04045f ? srgb / 12.92f :
               srgb > 0.4045f ? std::pow((srgb + 0.055f) / 1.055f, GAMMA_DECODE) :
               1.0f;
    }
    
    static glm::vec3 srgb_to_linear(const glm::vec3& srgb) {
        return glm::vec3(
            srgb_to_linear(srgb.r),
            srgb_to_linear(srgb.g),
            srgb_to_linear(srgb.b)
        );
    }
    
    // Linear to sRGB
    static float linear_to_srgb(float linear) {
        return linear <= 0.0031308f ? linear * 12.92f :
               linear > 0.0031308f ? 1.055f * std::pow(linear, GAMMA_ENCODE) - 0.055f :
               1.0f;
    }
    
    static glm::vec3 linear_to_srgb(const glm::vec3& linear) {
        return glm::vec3(
            linear_to_srgb(linear.r),
            linear_to_srgb(linear.g),
            linear_to_srgb(linear.b)
        );
    }
    
    // Rec.709 to linear
    static glm::mat3 rec709_to_linear_matrix() {
        return glm::mat3(
            REC709_R.r, REC709_G.r, REC709_B.r,
            REC709_R.g, REC709_G.g, REC709_B.g,
            REC709_R.b, REC709_G.b, REC709_B.b
        );
    }
    
    // Linear to Rec.709
    static glm::mat3 linear_to_rec709_matrix() {
        return glm::inverse(rec709_to_linear_matrix());
    }
    
    // Color space conversion
    static glm::vec3 rgb_to_hsv(const glm::vec3& rgb);
    static glm::vec3 hsv_to_rgb(const glm::vec3& hsv);
    static glm::vec3 rgb_to_yuv(const glm::vec3& rgb);
    static glm::vec3 yuv_to_rgb(const glm::vec3& yuv);
};

// Image processing utilities
struct ImageProcessingUtils {
    // Gaussian blur kernel generation
    static std::vector<float> generate_gaussian_kernel(float sigma, int kernel_size) {
        std::vector<float> kernel(kernel_size * kernel_size);
        float sigma_sq = sigma * sigma;
        float two_sigma_sq = 2.0f * sigma_sq;
        float pi_times_sigma_sq = ShaderConstants::PI * sigma_sq;
        float sum = 0.0f;
        
        int center = kernel_size / 2;
        for (int y = 0; y < kernel_size; ++y) {
            for (int x = 0; x < kernel_size; ++x) {
                int dx = x - center;
                int dy = y - center;
                float weight = std::exp(-(dx*dx + dy*dy) / two_sigma_sq);
                kernel[y * kernel_size + x] = weight;
                sum += weight;
            }
        }
        
        // Normalize kernel
        for (auto& weight : kernel) {
            weight /= sum;
        }
        
        return kernel;
    }
    
    // Box blur kernel generation
    static std::vector<float> generate_box_kernel(int kernel_size) {
        std::vector<float> kernel(kernel_size * kernel_size, 1.0f / (kernel_size * kernel_size));
        return kernel;
    }
    
    // Sharpen kernel generation
    static std::vector<float> generate_sharpen_kernel(float amount, int kernel_size) {
        std::vector<float> kernel(kernel_size * kernel_size, 0.0f);
        int center = kernel_size / 2;
        
        // Center pixel
        kernel[center * kernel_size + center] = 1.0f + 4.0f * amount;
        
        // Cross pattern for sharpening
        if (amount > 0.0f) {
            kernel[(center-1) * kernel_size + center] = -amount;
            kernel[(center+1) * kernel_size + center] = -amount;
            kernel[center * kernel_size + (center-1)] = -amount;
            kernel[center * kernel_size + (center+1)] = -amount;
        }
        
        return kernel;
    }
    
    // Unsharp mask kernel
    static std::vector<float> generate_unsharp_mask_kernel(float sigma, float amount, int kernel_size) {
        auto blur_kernel = generate_gaussian_kernel(sigma, kernel_size);
        auto sharpen_kernel = generate_sharpen_kernel(amount, kernel_size);
        
        std::vector<float> mask_kernel(kernel_size * kernel_size);
        for (size_t i = 0; i < kernel.size(); ++i) {
            mask_kernel[i] = blur_kernel[i] - sharpen_kernel[i];
        }
        
        return mask_kernel;
    }
    
    // Bilateral filter helpers
    static float bilateral_weight(float spatial_distance, float intensity_diff, float sigma_s, float sigma_r) {
        float spatial_weight = std::exp(-(spatial_distance * spatial_distance) / (2.0f * sigma_s * sigma_s));
        float range_weight = std::exp(-(intensity_diff * intensity_diff) / (2.0f * sigma_r * sigma_r));
        return spatial_weight * range_weight;
    }
    
    // Morphological operations
    enum class MorphOp { Erode, Dilate, Open, Close, Gradient };
    
    static bool is_edge_pixel(const std::vector<float>& neighborhood, int center_index, MorphOp operation) {
        float center = neighborhood[center_index];
        
        for (size_t i = 0; i < neighborhood.size(); ++i) {
            if (i == center_index) continue;
            
            bool neighbor_pixel = neighborhood[i] > 0.5f;
            
            switch (operation) {
                case MorphOp::Erode:
                    if (!neighbor_pixel && center > 0.5f) return true;
                    break;
                case MorphOp::Dilate:
                    if (neighbor_pixel && center <= 0.5f) return true;
                    break;
                case MorphOp::Open:
                    if (!neighbor_pixel && center > 0.5f) return true;
                    break;
                case MorphOp::Close:
                    if (neighbor_pixel && center <= 0.5f) return true;
                    break;
                default:
                    break;
            }
        }
        
        return false;
    }
};

// Texture sampling utilities
struct TextureSamplingUtils {
    // Bilinear interpolation
    static glm::vec4 sample_bilinear(const glm::vec2& uv, const glm::vec2& texture_size,
                                   const std::function<glm::vec4(const glm::vec2&)>& sampler) {
        glm::vec2 texel = uv * texture_size - glm::vec2(0.5f);
        glm::vec2 texel_floored = glm::floor(texel);
        glm::vec2 texel_fract = glm::fract(texel);
        
        glm::vec2 texel1 = texel_floored + glm::vec2(1.0f, 0.0f);
        glm::vec2 texel2 = texel_floored + glm::vec2(0.0f, 1.0f);
        
        glm::vec4 sample00 = sampler(texel1 / texture_size);
        glm::vec4 sample01 = sampler(texel2 / texture_size);
        glm::vec4 sample10 = sampler((texel1 + glm::vec2(1.0f, 0.0f)) / texture_size);
        glm::vec4 sample11 = sampler((texel1 + glm::vec2(1.0f, 1.0f)) / texture_size);
        
        return glm::mix(
            glm::mix(sample00, sample01, texel_fract.x),
            glm::mix(sample10, sample11, texel_fract.y)
        );
    }
    
    // Bicubic interpolation
    static glm::vec4 sample_bicubic(const glm::vec2& uv, const glm::vec2& texture_size,
                                   const std::function<glm::vec4(const glm::vec2&)>& sampler) {
        glm::vec2 texel = uv * texture_size - glm::vec2(0.5f);
        glm::vec2 texel_floored = glm::floor(texel);
        glm::vec2 texel_fract = glm::fract(texel);
        
        glm::vec2 weights[4];
        weights[0] = cubic_weight(texel_fract.x - 1.0f);
        weights[1] = cubic_weight(texel_fract.x);
        weights[2] = cubic_weight(1.0f - texel_fract.x);
        weights[3] = cubic_weight(2.0f - texel_fract.x);
        
        glm::vec4 samples[16];
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                glm::vec2 sample_pos = (texel_floored + glm::vec2(x - 1, y - 1)) / texture_size;
                samples[y * 4 + x] = sampler(sample_pos);
            }
        }
        
        return bicubic_interpolate(samples, weights, texel_fract.y, weights);
    }
    
private:
    static float cubic_weight(float t) {
        return t <= 0.0f ? 0.0f :
               t >= 1.0f ? 0.0f :
               t >= 2.0f ? 0.0f :
               t * t * (3.0f - 2.0f * t);
    }
    
    static glm::vec4 bicubic_interpolate(const glm::vec4 samples[16], 
                                  const glm::vec2 weights_x[4], float weight_y,
                                  const glm::vec2 weights_y[4], float weight_x) {
        glm::vec4 row0 = bicubic_interpolate_row(samples, weights_x, 0, weight_y);
        glm::vec4 row1 = bicubic_interpolate_row(samples, weights_x, 4, weight_y);
        glm::vec4 row2 = bicubic_interpolate_row(samples, weights_x, 8, weight_y);
        glm::vec4 row3 = bicubic_interpolate_row(samples, weights_x, 12, weight_y);
        
        return bicubic_interpolate_row({row0, row1, row2, row3}, weights_y, weight_x);
    }
    
    static glm::vec4 bicubic_interpolate_row(const glm::vec4 samples[4], 
                                          const glm::vec2 weights[4], int start_index,
                                          float weight) {
        return (samples[start_index] * weights[0] +
                samples[start_index + 1] * weights[1] +
                samples[start_index + 2] * weights[2] +
                samples[start_index + 3] * weights[3]) * weight;
    }
};

// Noise generation utilities
struct NoiseUtils {
    // Pseudo-random number generator (for reproducible noise)
    struct PRNG {
        uint32_t seed;
        
        PRNG(uint32_t seed = 12345) : seed(seed) {}
        
        uint32_t random() {
            seed = seed * 1103515245 + 12345;
            return seed ^ (seed >> 16);
        }
        
        float random_float() {
            return static_cast<float>(random()) / static_cast<float>(UINT32_MAX);
        }
        
        glm::vec2 random_vec2() {
            return glm::vec2(random_float(), random_float());
        }
        
        glm::vec3 random_vec3() {
            return glm::vec3(random_float(), random_float(), random_float());
        }
        
        glm::vec4 random_vec4() {
            return glm::vec4(random_float(), random_float(), random_float(), random_float());
        }
    };
    
    // Perlin noise
    static float perlin_noise_2d(const glm::vec2& p, PRNG& rng) {
        glm::vec2 i = glm::floor(p);
        glm::vec2 f = glm::fract(p);
        
        // Hash function
        auto hash = [&rng](const glm::vec2& v) -> uint32_t {
            uint32_t h = std::hash<float>{}(v.x) ^ std::hash<float>{}(v.y);
            return h;
        };
        
        // Corner gradients
        glm::vec2 n00 = glm::vec2(hash(i + glm::vec2(0,0)) / UINT32_MAX, 
                           hash(i + glm::vec2(1,0)) / UINT32_MAX);
        glm::vec2 n10 = glm::vec2(hash(i + glm::vec2(0,1)) / UINT32_MAX, 
                           hash(i + glm::vec2(1,1)) / UINT32_MAX);
        
        glm::vec2 n01 = glm::vec2(hash(i + glm::vec2(1,0)) / UINT32_MAX, 
                           hash(i + glm::vec2(2,0)) / UINT32_MAX);
        glm::vec2 n11 = glm::vec2(hash(i + glm::vec2(1,1)) / UINT32_MAX, 
                           hash(i + glm::vec2(2,1)) / UINT32_MAX);
        
        // Fade curves
        glm::vec2 u = f * f * (3.0f - 2.0f * f);
        glm::vec2 n = n00 + u.x * (n10 - n00) + u.y * (n01 - n00);
        
        // Noise value
        return 2.0f * n.y + -1.0f;
    }
    
    // Fractal brownian motion
    static glm::vec3 fbm_noise_3d(const glm::vec3& p, int octaves, float persistence, float lacunarity, PRNG& rng) {
        glm::vec3 value = glm::vec3(0.0f);
        float amplitude = 1.0f;
        float frequency = 1.0f;
        
        for (int i = 0; i < octaves; ++i) {
            glm::vec3 offset = glm::vec3(rng.random_float(), rng.random_float(), rng.random_float());
            glm::vec3 noise = glm::vec3(perlin_noise_2d(p.xy * frequency + offset.xy, rng) * frequency + offset.z,
                                       perlin_noise_2d(p.yz * frequency + offset.xy, rng) * frequency + offset.z));
            
            value += noise * amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        
        return value;
    }
    
    // Simple noise patterns
    static float value_noise_2d(const glm::vec2& p, PRNG& rng) {
        return rng.random_float(); // Simple random noise
    }
    
    static float cellular_noise_2d(const glm::vec2& p, PRNG& rng) {
        glm::vec2 cell = glm::floor(p);
        return rng.random_float() < 0.5f ? 0.0f : 1.0f;
    }
};

// Film grain simulation
struct FilmGrainUtils {
    // Grain types
    enum class GrainType {
        Uniform,
        Gaussian,
        Exponential,
        Poisson
    };
    
    // Film grain parameters
    struct GrainParameters {
        float amount = 0.05f;
        float size = 1.0f;
        GrainType type = GrainType::Uniform;
        bool colored = false;
        float seed = 12345.0f;
        
        // Color-specific parameters
        glm::vec3 color_balance = glm::vec3(1.0f);
        float saturation = 1.0f;
        
        // Size variation
        float size_variation = 0.2f;
        
        // Density
        float density = 1.0f;
        float density_randomness = 0.0f;
    };
    
    // Generate grain texture
    static void generate_grain_texture(const GrainParameters& params, 
                                  std::vector<float>& texture_data,
                                  int width, int height) {
        texture_data.resize(width * height * 4); // RGBA
        
        PRNG rng(static_cast<uint32_t>(params.seed));
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                glm::vec3 grain;
                
                switch (params.type) {
                    case GrainType::Uniform:
                        grain = glm::vec3(rng.random_float());
                        break;
                        
                    case GrainType::Gaussian:
                        {
                            float u1 = rng.random_float();
                            float u2 = rng.random_float();
                            float gaussian = std::exp(-(u1*u1 + u2*u2) / 2.0f);
                            grain = glm::vec3(gaussian, gaussian, gaussian) * 0.5f + 0.5f;
                        }
                        break;
                        
                    case GrainType::Exponential:
                        grain = glm::vec3(-std::log(1.0f - rng.random_float()) * params.amount);
                        break;
                        
                    case GrainType::Poisson:
                        if (rng.random_float() < params.density) {
                            grain = glm::vec3(1.0f);
                        } else {
                            grain = glm::vec3(0.0f);
                        }
                        break;
                        
                    default:
                        grain = glm::vec3(0.5f);
                        break;
                }
                
                // Apply color balance
                grain *= params.color_balance;
                
                // Apply size variation
                float size_var = 1.0f;
                if (params.size_variation > 0.0f) {
                    size_var = 1.0f + (rng.random_float() - 0.5f) * 2.0f * params.size_variation;
                }
                
                // Apply density randomness
                if (params.density_randomness > 0.0f && rng.random_float() < params.density) {
                    grain = glm::vec3(0.0f);
                }
                
                // Apply saturation
                float gray_value = (grain.r * 0.299f + grain.g * 0.587f + grain.b * 0.114f);
                glm::vec3 saturated_grain = gray_value + (grain - gray_value) * params.saturation;
                
                // Final grain value
                glm::vec3 final_grain = saturated_grain * params.amount * size_var;
                float alpha = params.colored ? 1.0f : 0.0f;
                
                texture_data[(y * width + x) * 4 + 0] = final_grain.r;
                texture_data[(y * width + x) * 4 + 1] = final_grain.g;
                texture_data[(y * width + x) * 4 + 2] = final_grain.b;
                texture_data[(y * width + x) * 4 + 3] = alpha;
            }
        }
    }
    
    // Apply grain to image
    static void apply_grain(const std::vector<float>& grain_texture, 
                     const std::vector<float>& input_image,
                     std::vector<float>& output_image,
                     const GrainParameters& params) {
        size_t pixel_count = input_image.size() / 4; // Assuming RGBA
        
        for (size_t i = 0; i < pixel_count; ++i) {
            float input_r = input_image[i * 4 + 0];
            float input_g = input_image[i * 4 + 1];
            float input_b = input_image[i * 4 + 2];
            float input_a = input_image[i * 4 + 3];
            
            // Sample grain at pixel position
            int grain_x = static_cast<int>(i % grain_texture.size() / 4);
            int grain_y = static_cast<int>(i / (grain_texture.size() / 4));
            
            float grain_r = grain_texture[grain_y * grain_texture.size() / 4 + grain_x * 4 + 0];
            float grain_g = grain_texture[grain_y * grain_texture.size() / 4 + grain_x * 4 + 1];
            float grain_b = grain_texture[grain_y * grain_texture.size() / 4 + grain_x * 4 + 2];
            float grain_a = grain_texture[grain_y * grain_texture.size() / 4 + grain_x * 4 + 3];
            
            // Apply grain (multiply blend)
            output_image[i * 4 + 0] = input_r * (1.0f - grain_r + grain_r);
            output_image[i * 4 + 1] = input_g * (1.0f - grain_g + grain_g);
            output_image[i * 4 + 2] = input_b * (1.0f - grain_b + grain_b);
            output_image[i * 4 + 3] = input_a * (1.0f - grain_a + grain_a);
        }
    }
};

} // namespace Shotcut::Effects
```

---

## 8.2 COLOR CORRECTION SYSTEM

### 8.2.1 Professional Color Grading Tools

**Color Correction Philosophy:**
- **Broadcast Quality:** Professional broadcast-quality color correction
- **Real-Time Performance:** 60fps color grading even with 4K footage
- **Comprehensive Tools:** Complete set of color grading tools
- **Industry Standards:** Support for Rec.709, Rec.2020, and DCI-P3 color spaces

**src/effects/color_correction.h:**
```cpp
#pragma once

#include "effects_framework.h"
#include "shader_library.h"
#include <glm/glm.hpp>

namespace Shotcut::Effects {

// Color correction parameters
struct ColorCorrectionParams {
    // Basic adjustments
    float brightness = 0.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    float gamma = 1.0f;
    
    // Color balance
    float red_balance = 1.0f;
    float green_balance = 1.0f;
    float blue_balance = 1.0f;
    
    // Color wheels
    glm::vec3 master_wheel = glm::vec3(0.0f);
    glm::vec3 shadow_wheel = glm::vec3(0.0f);
    glm::vec3 midtone_wheel = glm::vec3(0.0f);
    glm::vec3 highlight_wheel = glm::vec3(0.0f);
    
    // Curves
    struct CurvePoint {
        float input = 0.0f;
        float output = 0.0f;
    };
    
    std::vector<CurvePoint> red_curve;
    std::vector<CurvePoint> green_curve;
    std::vector<CurvePoint> blue_curve;
    std::vector<CurvePoint> master_curve;
    
    // Selective color
    glm::vec4 selective_color = glm::vec4(1.0f); // Color to preserve
    float selective_range = 0.0f;       // Color range to affect
    
    // Color space
    enum class ColorSpace {
        SRGB,
        Rec709,
        Rec2020,
        DCI_P3
    } working_color_space = ColorSpace::SRGB;
    
    // LUT parameters
    bool use_lut = false;
    std::string lut_path;
    float lut_strength = 1.0f;
    
    // Performance settings
    bool real_time_preview = true;
    bool use_optimized_shaders = true;
};

// Color correction effect implementation
class ColorCorrectionEffect : public Effect {
private:
    ColorCorrectionParams params_;
    
    // GPU resources for color correction
    struct ColorCorrectionUniforms {
        glm::vec4 color_params;        // brightness, contrast, saturation, gamma
        glm::vec3 color_balance;        // r, g, b balance
        glm::vec3 color_wheels[4];     // master, shadow, midtone, highlight
        
        // Curve data
        struct CurveData {
            glm::vec4 points[256]; // RGBA curves
        } curves[4]; // r, g, b, master
        
        // Selective color
        glm::vec4 selective_color;
        float selective_range;
        
        // LUT data
        int use_lut;
        float lut_strength;
        
        // Color space
        int working_color_space;
        
        // Padding
        float padding[8];
    };
    
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // Color curves texture
    struct ColorCurvesTexture {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkSampler sampler;
        
        bool needs_update = true;
    };
    
    ColorCurvesTexture curves_texture_;
    
    // LUT texture
    struct LUTTexture {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkSampler sampler;
        
        bool needs_update = true;
        int lut_size = 0; // 1D texture size
    };
    
    LUTTexture lut_texture_;
    
public:
    ColorCorrectionEffect();
    ~ColorCorrectionEffect() override;
    
    // Effect interface implementation
    bool initialize(VkDevice device, VkQueue compute_queue) override;
    void cleanup() override;
    
    void set_parameter(const std::string& name, const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value) override;
    
    bool process(const GPUBuffer& input, GPUBuffer& output, 
                const glm::ivec2& size, ProcessingStage stage) override;
    
    // Color correction specific methods
    void set_color_correction_params(const ColorCorrectionParams& params);
    const ColorCorrectionParams& get_color_correction_params() const { return params_; }
    
    void set_brightness(float brightness);
    void set_contrast(float contrast);
    void set_saturation(float saturation);
    void set_gamma(float gamma);
    
    void set_color_balance(float red, float green, float blue);
    void set_color_wheel(int wheel_index, const glm::vec3& values);
    
    void add_curve_point(int curve_index, const CurvePoint& point);
    void remove_curve_point(int curve_index, int point_index);
    void set_curve_points(int curve_index, const std::vector<CurvePoint>& points);
    
    void set_selective_color(const glm::vec4& color, float range);
    void set_working_color_space(ColorSpace color_space);
    
    void load_lut(const std::string& file_path);
    void set_lut_strength(float strength);
    
    // Preset management
    void apply_preset(const std::string& preset_name);
    void save_preset(const std::string& preset_name);
    void reset_to_defaults();
    
    std::vector<std::string> get_available_presets() const;
    
private:
    bool create_gpu_resources();
    void destroy_gpu_resources();
    
    bool create_uniform_buffer();
    bool create_descriptor_set();
    
    bool create_curves_texture();
    bool create_lut_texture();
    
    void update_uniform_buffer();
    void update_curves_texture();
    void update_lut_texture();
    
    void upload_curve_data_to_gpu();
    void upload_lut_data_to_gpu();
    
    // Color correction compute shader
    std::string generate_color_correction_shader();
    
    // Curve interpolation
    glm::vec4 interpolate_curve(const std::vector<CurvePoint>& points, float input) const;
    std::vector<glm::vec4> sample_curve_texture(const std::vector<CurvePoint>& points) const;
    
    // Color space conversion
    glm::vec3 apply_color_space_conversion(const glm::vec3& color) const;
    
    // LUT sampling
    glm::vec3 sample_lut(const glm::vec3& color, float strength) const;
};

// Vector scope effect for color analysis
class VectorScopeEffect : public Effect {
private:
    struct VectorScopeParams {
        float exposure = 0.0f;
        bool false_color = false;
        glm::vec4 target_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        bool show_parade = false;
        bool show_vectors = true;
        
        enum class ScopeType {
            Histogram,
            Parade,
            Vectors,
            Waveform
        } scope_type = ScopeType::Vectors;
        
        int parade_rows = 3; // YUV parade
    };
    
    VectorScopeParams params_;
    
    // GPU resources for vector scope
    struct VectorScopeUniforms {
        glm::vec4 target_color;
        glm::vec2 target_xy; // For false color analysis
        float exposure;
        int scope_type;
        int parade_rows;
        
        // Histogram data
        glm::vec4 histogram[256];
        
        // Padding
        float padding[8];
    };
    
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // Histogram texture
    VkImage histogram_image_;
    VkImageView histogram_view_;
    VkDeviceMemory histogram_memory_;
    
    // Scope rendering
    VkBuffer vertex_buffer_;
    VkBuffer index_buffer_;
    VkDeviceMemory vertex_memory_;
    VkDeviceMemory index_memory_;
    
    uint32_t vertex_count_;
    uint32_t index_count_;
    
public:
    VectorScopeEffect();
    ~VectorScopeEffect() override;
    
    bool initialize(VkDevice device, VkQueue compute_queue) override;
    void cleanup() override;
    
    void set_parameter(const std::string& name, const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value) override;
    
    bool process(const GPUBuffer& input, GPUBuffer& output, 
                const glm::ivec2& size, ProcessingStage stage) override;
    
    // Vector scope specific methods
    void set_exposure(float exposure);
    void set_target_color(const glm::vec4& color);
    void set_scope_type(VectorScopeParams::ScopeType type);
    
    void set_parade_configuration(int rows, bool show_vectors = false);
    void set_histogram_analysis(bool enabled);
    
    std::vector<float> get_histogram_data() const;
    std::vector<std::vector<float>> get_parade_data() const;
    std::vector<std::vector<float>> get_vector_data() const;
    
private:
    bool create_gpu_resources();
    void destroy_gpu_resources();
    
    bool create_uniform_buffer();
    bool create_descriptor_set();
    bool create_histogram_image();
    bool create_scope_geometry();
    
    void update_uniform_buffer();
    void update_histogram_data();
    
    void generate_scope_vertices();
    
    // Vector scope compute shader
    std::string generate_histogram_shader();
    std::string generate_parade_shader();
    std::string generate_vectors_shader();
    
    // Histogram computation
    void compute_histogram(const GPUBuffer& input, const glm::ivec2& size);
    
    // Parade and vector rendering
    void render_parade(VkCommandBuffer command_buffer);
    void render_vectors(VkCommandBuffer command_buffer);
};

} // namespace Shotcut::Effects
```

---

## 8.3 IMAGE ADJUSTMENT EFFECTS

### 8.3.1 Blur and Sharpen Effects

**Image Adjustment Philosophy:**
- **High Quality:** Professional-grade blur and sharpen algorithms
- **Real-Time Performance:** GPU-accelerated processing with 60fps preview
- **Multiple Algorithms:** Gaussian, box, motion blur, unsharp mask, etc.
- **Customizable Parameters:** Fine-grained control over effect parameters

**src/effects/image_adjustment.h:**
```cpp
#pragma once

#include "effects_framework.h"
#include "shader_library.h"
#include <glm/glm.hpp>

namespace Shotcut::Effects {

// Blur types
enum class BlurType {
    Gaussian,
    Box,
    Motion,
    Radial,
    Zoom,
    Surface
};

// Blur parameters
struct BlurParams {
    BlurType type = BlurType::Gaussian;
    float radius = 5.0f;
    float sigma = 1.0f;
    
    // Advanced parameters
    int passes = 1;
    float angle = 0.0f; // For directional blurs
    float anisotropy = 0.0f; // For elliptical blurs
    
    // Motion blur specific
    glm::vec2 motion_vector = glm::vec2(0.0f);
    float motion_length = 0.0f;
    
    // Performance settings
    bool use_optimized_kernel = true;
    bool downsample_for_large_blurs = true;
};

// Sharpen parameters
struct SharpenParams {
    float amount = 0.5f;
    float radius = 1.0f;
    float threshold = 0.0f;
    
    // Unsharp mask parameters
    bool use_unsharp_mask = false;
    float mask_sigma = 1.0f;
    float mask_amount = 1.0f;
    
    // Performance settings
    bool use_separable_sharpen = true;
};

// Combined blur and sharpen effect
class BlurSharpenEffect : public Effect {
private:
    BlurParams blur_params_;
    SharpenParams sharpen_params_;
    
    // GPU resources
    struct EffectUniforms {
        // Blur parameters
        struct {
            int blur_type;
            float radius;
            float sigma;
            float angle;
            float anisotropy;
            int passes;
            glm::vec2 motion_vector;
            float motion_length;
            
            // Gaussian kernel parameters
            int kernel_size;
            std::vector<float> kernel;
        } blur;
        
        // Sharpen parameters
        struct {
            float amount;
            float radius;
            float threshold;
            
            // Unsharp mask parameters
            int use_unsharp_mask;
            float mask_sigma;
            float mask_amount;
            float mask_kernel[25]; // 5x5 kernel
            
            // Performance settings
            int use_separable_sharpen;
        } sharpen;
        
        // Combined parameters
        int effect_mode; // 0 = blur only, 1 = sharpen only, 2 = combined
        float blur_strength = 1.0f;
        float sharpen_strength = 1.0f;
        
        // Padding
        float padding[8];
    };
    
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // Kernel textures
    struct KernelTexture {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkSampler sampler;
        
        bool needs_update = true;
        int kernel_size = 0;
    };
    
    KernelTexture blur_kernel_texture_;
    KernelTexture unsharp_kernel_texture_;
    
    // Temporal filtering for motion blur
    struct TemporalFilter {
        VkImage input_textures[2]; // Previous and current frame
        VkImageView input_views[2];
        VkSampler input_samplers[2];
        
        VkImage output_texture;
        VkImageView output_view;
        VkDeviceMemory output_memory;
        
        int frame_index = 0;
        float temporal_weight = 0.5f;
    };
    
    TemporalFilter temporal_filter_;
    
public:
    BlurSharpenEffect();
    ~BlurSharpenEffect() override;
    
    bool initialize(VkDevice device, VkQueue compute_queue) override;
    void cleanup() override;
    
    void set_parameter(const std::string& name, const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value) override;
    
    bool process(const GPUBuffer& input, GPUBuffer& output, 
                const glm::ivec2& size, ProcessingStage stage) override;
    
    // Blur specific methods
    void set_blur_params(const BlurParams& params);
    const BlurParams& get_blur_params() const { return blur_params_; }
    
    void set_blur_type(BlurType type);
    void set_blur_radius(float radius);
    void set_blur_sigma(float sigma);
    void set_motion_blur(const glm::vec2& vector, float length);
    
    // Sharpen specific methods
    void set_sharpen_params(const SharpenParams& params);
    const SharpenParams& get_sharpen_params() const { return sharpen_params_; }
    
    void set_sharpen_amount(float amount);
    void set_unsharp_mask(bool enabled, float sigma, float amount);
    
    // Combined effect methods
    void set_effect_mode(int mode); // 0 = blur, 1 = sharpen, 2 = combined
    void set_strengths(float blur_strength, float sharpen_strength);
    
    // Preset management
    void apply_blur_preset(const std::string& preset_name);
    void apply_sharpen_preset(const std::string& preset_name);
    
    std::vector<std::string> get_blur_presets() const;
    std::vector<std::string> get_sharpen_presets() const;
    
private:
    bool create_gpu_resources();
    void destroy_gpu_resources();
    
    bool create_uniform_buffer();
    bool create_descriptor_set();
    bool create_kernel_textures();
    bool create_temporal_filter();
    
    void update_uniform_buffer();
    void update_kernel_textures();
    void update_temporal_filter();
    
    // Blur kernels
    std::vector<float> generate_gaussian_kernel(float sigma, int kernel_size) const;
    std::vector<float> generate_box_kernel(int kernel_size) const;
    std::vector<float> generate_motion_blur_kernel(const glm::vec2& vector, int kernel_size) const;
    
    // Sharpen kernels
    std::vector<float> generate_sharpen_kernel(float radius) const;
    std::vector<float> generate_unsharp_mask_kernel(float sigma, float amount, int kernel_size) const;
    
    // Blur and sharpen shaders
    std::string generate_blur_shader(BlurType type);
    std::string generate_sharpen_shader();
    std::string generate_combined_shader();
    
    // Kernel texture updates
    void upload_blur_kernel_to_gpu();
    void upload_sharpen_kernel_to_gpu();
    
    // Temporal filtering
    void update_temporal_frame(const GPUBuffer& input);
};

// Advanced blur variants
class DirectionalBlurEffect : public BlurSharpenEffect {
public:
    DirectionalBlurEffect();
    
    void set_directional_blur(float angle, float anisotropy);
};

class MotionBlurEffect : public BlurSharpenEffect {
public:
    MotionBlurEffect();
    
    void set_motion_blur(const glm::vec2& vector, float length);
};

class RadialBlurEffect : public BlurSharpenEffect {
public:
    RadialBlurEffect();
    
    void set_radial_blur(float center_strength, float edge_strength);
};

class ZoomBlurEffect : public BlurSharpenEffect {
public:
    ZoomBlurEffect();
    
    void set_zoom_blur(float center_strength, float edge_strength);
};

// Noise reduction with advanced blur
class NoiseReductionEffect : public Effect {
private:
    struct NoiseReductionParams {
        float spatial_strength = 0.5f;
        float temporal_strength = 0.3f;
        float threshold = 0.1f;
        
        bool use_edge_preservation = true;
        float edge_threshold = 0.05f;
        
        // Advanced settings
        int spatial_passes = 2;
        bool use_bilateral_filter = false;
        float bilateral_sigma = 1.0f;
    };
    
    NoiseReductionParams params_;
    
public:
    NoiseReductionEffect();
    
    void set_noise_reduction_params(const NoiseReductionParams& params);
    void enable_bilateral_filter(bool enable);
    void set_edge_preservation(bool preserve);
    
protected:
    bool initialize(VkDevice device, VkQueue compute_queue) override;
    
    std::string generate_noise_reduction_shader();
    void update_noise_reduction_uniforms();
};

} // namespace Shotcut::Effects
```

---

## 8.4 COMPOSITE AND TRANSITION EFFECTS

### 8.4.1 Professional Compositing System

**Compositing Philosophy:**
- **Professional Blending:** Industry-standard blend modes and compositing
- **High Performance:** GPU-accelerated compositing with minimal overhead
- **Advanced Features:** Keying, mattes, advanced color spaces
- **Real-Time Preview:** 60fps compositing even with complex scenes

**src/effects/composite_effects.h:**
```cpp
#pragma once

#include "effects_framework.h"
#include "shader_library.h"
#include <glm/glm.hpp>

namespace Shotcut::Effects {

// Blend modes
enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity
};

// Composite parameters
struct CompositeParams {
    BlendMode blend_mode = BlendMode::Normal;
    float opacity = 1.0f;
    
    // Advanced blending
    float blend_factor = 1.0f;
    glm::vec4 blend_color = glm::vec4(0.0f);
    
    // Color space
    bool convert_to_linear = false;
    bool convert_to_srgb = false;
    
    // Performance settings
    bool use_optimized_blend_mode = true;
};

// Keying parameters
struct KeyingParams {
    enum class KeyType {
        Luma,
        Chroma,
        Alpha,
        RGB
        RGBA
    } key_type = KeyType::Alpha;
    
    float key_threshold = 0.5f;
    float key_feather = 0.01f;
    
    bool use_inverted_key = false;
    bool use_hard_matte = true;
    
    // Advanced keying
    float key_black_point = 0.0f;
    float key_white_point = 1.0f;
    float key_gamma = 1.0f;
    
    // Key color
    glm::vec4 key_color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Edge detection for advanced keying
    bool use_edge_detection = false;
    float edge_threshold = 0.1f;
    float edge_feather = 0.01f;
};

// Matte parameters
struct MatteParams {
    // Garbage matte
    bool use_garbage_matte = false;
    float garbage_threshold = 0.1f;
    float garbage_feather = 0.01f;
    
    // Edge matte
    bool use_edge_matte = false;
    float edge_threshold = 0.5f;
    float edge_feather = 0.01f;
    
    // Core matte
    float core_threshold = 0.5f;
    float core_feather = 0.01f;
    
    // Matte color
    glm::vec4 matte_color = glm::vec4(0.0f);
    
    // Inverted matte
    bool invert_matte = false;
};

// Composite effect implementation
class CompositeEffect : public Effect {
private:
    CompositeParams composite_params_;
    KeyingParams keying_params_;
    MatteParams matte_params_;
    
    // GPU resources
    struct CompositeUniforms {
        // Blend parameters
        struct {
            int blend_mode;
            float opacity;
            float blend_factor;
            glm::vec4 blend_color;
            
            int convert_to_linear;
            int convert_to_srgb;
            
            // Performance settings
            int use_optimized_blend_mode;
        } blend;
        
        // Keying parameters
        struct {
            int key_type;
            float key_threshold;
            float key_feather;
            
            int use_inverted_key;
            int use_hard_matte;
            
            float key_black_point;
            float key_white_point;
            float key_gamma;
            
            // Key color
            glm::vec4 key_color;
            
            // Advanced keying
            int use_edge_detection;
            float edge_threshold;
            float edge_feather;
            
            // Padding
            float key_padding[8];
        } keying;
        
        // Matte parameters
        struct {
            int use_garbage_matte;
            float garbage_threshold;
            float garbage_feather;
            
            int use_edge_matte;
            float edge_threshold;
            float edge_feather;
            
            float core_threshold;
            float core_feather;
            
            // Matte color
            glm::vec4 matte_color;
            
            int invert_matte;
            
            // Padding
            float matte_padding[8];
        } matte;
        
        // Padding
        float padding[16];
    };
    
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // GPU textures for advanced processing
    struct ProcessingTextures {
        // Edge detection texture
        VkImage edge_texture;
        VkImageView edge_view;
        VkDeviceMemory edge_memory;
        VkSampler edge_sampler;
        
        bool needs_update = true;
        
        // Matte textures
        VkImage garbage_matte_texture;
        VkImage edge_matte_texture;
        VkImage core_matte_texture;
        VkImageView matte_views[4];
        VkDeviceMemory matte_memorys[4];
        VkSampler matte_samplers[4];
        
        bool needs_update[4];
    };
    
    ProcessingTextures processing_textures_;
    
public:
    CompositeEffect();
    ~CompositeEffect() override;
    
    bool initialize(VkDevice device, VkQueue compute_queue) override;
    void cleanup() override;
    
    void set_parameter(const std::string& name, const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value) override;
    
    // Blend mode methods
    void set_blend_mode(BlendMode mode);
    void set_opacity(float opacity);
    void set_blend_factor(float factor);
    void set_blend_color(const glm::vec4& color);
    
    // Color space conversion
    void set_color_space_conversion(bool to_linear, bool to_srgb);
    
    // Keying methods
    void set_keying_params(const KeyingParams& params);
    void set_key_type(KeyingParams::KeyType type);
    void set_key_threshold(float threshold);
    void set_key_feather(float feather);
    
    void set_key_color(const glm::vec4& color);
    void set_key_range(float black_point, float white_point, float gamma);
    
    void enable_edge_keying(bool enable);
    void enable_hard_matte(bool enable);
    
    // Matte methods
    void set_garbage_matte(bool enabled, float threshold, float feather);
    void set_edge_matte(bool enabled, float threshold, float feather);
    void set_core_matte(float threshold, float feather);
    
    void set_matte_color(const glm::vec4& color);
    void invert_matte(bool invert);
    
    // Preset management
    void apply_blend_preset(const std::string& preset_name);
    void apply_keying_preset(const std::string& preset_name);
    void apply_matte_preset(const std::string& preset_name);
    
    std::vector<std::string> get_blend_presets() const;
    std::vector<std::string> get_keying_presets() const;
    std::vector<std::string> get_matte_presets() const;
    
private:
    bool create_gpu_resources();
    void destroy_gpu_resources();
    
    bool create_uniform_buffer();
    bool create_descriptor_set();
    bool create_processing_textures();
    
    void update_uniform_buffer();
    void update_processing_textures();
    
    // Edge detection
    void compute_edge_detection(const GPUBuffer& input, const glm::ivec2& size);
    
    // Matte generation
    void compute_garbage_matte(const GPUBuffer& input, const glm::ivec2& size);
    void compute_edge_matte(const GPUBuffer& input, const glm::ivec2& size);
    void compute_core_matte(const GPUBuffer& input, const glm::ivec2& size);
    
    // Composite shaders
    std::string generate_blend_shader(BlendMode mode);
    std::string generate_keying_shader(KeyingParams::KeyType type);
    std::string generate_matte_shader(int matte_type);
    std::string generate_composite_shader();
    
    // Luminance computation for advanced keying
    float compute_luminance(const glm::vec4& color) const;
    float compute_luminance_with_gamma(const glm::vec4& color, float gamma) const;
};

// Transition effects
enum class TransitionType {
    None,
    CrossDissolve,
    DipToColor,
    Fade,
    Wipe,
    Iris,
    Box,
    Circle,
    Diamond,
    Clock,
    Zoom,
    Slide,
    Push,
    Cover
};

// Transition parameters
struct TransitionParams {
    TransitionType type = TransitionType::CrossDissolve;
    float duration = 1.0f; // Duration in seconds
    float position = 0.0f;   // Current position (0.0 to 1.0)
    
    // Transition-specific parameters
    union {
        struct {
            float smoothness = 0.5f; // For cross-dissolve
            int border_width = 0;
            glm::vec4 border_color = glm::vec4(0.0f);
        } cross_dissolve;
        
        struct {
            glm::vec4 color = glm::vec4(0.0f); // Color to dip to
            float hold_duration = 0.5f;
        } dip_to_color;
        
        struct {
            enum class FadeDirection {
                In, Out, Cross
            } direction = FadeDirection::Cross;
        } fade;
        
        struct {
            enum class WipeDirection {
                Left, Right, Up, Down,
                Diagonal
            } direction = WipeDirection::Left;
            
            float softness = 0.5f;
            int border_width = 0;
        } wipe;
        
        struct {
            float radius = 0.5f;
            float feather = 0.1f;
            
            enum class IrisShape {
                Circle, Square, Diamond, Triangle
            } shape = IrisShape::Circle;
        } iris;
        
        struct {
            float size = 0.5f;
            float angle = 0.0f;
            
            enum class BoxDirection {
                Horizontal, Vertical,
                Diagonal
            } direction = BoxDirection::Horizontal;
        } box;
        
        struct {
            float zoom_factor = 1.0f;
            glm::vec2 center = glm::vec2(0.5f);
            float rotation = 0.0f;
            
            enum class ClockDirection {
                Clockwise, CounterClockwise
            } direction = ClockDirection::Clockwise;
        } clock;
        
        struct {
            float angle = 45.0f;
            float distance = 0.5f;
            
            enum class PushDirection {
                Left, Right, Up, Down,
                Diagonal
            } direction = PushDirection::Left;
        } push;
        
        struct {
            enum class CoverDirection {
                Up, Down, Left, Right,
                Diagonal
            } direction = CoverDirection::Up;
        } cover;
        
        struct {
            float size = 1.0f;
            float amount = 1.0f;
            
            enum class SlideDirection {
                Left, Right, Up, Down,
                Diagonal
            } direction = SlideDirection::Left;
        } slide;
        
        struct {
            enum class ZoomDirection {
                In, Out
            } direction = ZoomDirection::In;
        } zoom;
    } transition_params;
    
    bool auto_reverse = true; // Auto-reverse at end
    bool use_custom_curve = false;
    std::vector<std::pair<float, float>> custom_curve;
};

// Transition effect implementation
class TransitionEffect : public Effect {
private:
    TransitionParams transition_params_;
    
    // GPU resources
    struct TransitionUniforms {
        // Common parameters
        float position;
        int transition_type;
        float duration;
        
        // Transition-specific parameters
        struct {
            float smoothness;
            int border_width;
            glm::vec4 border_color;
            float softness;
            float feather;
            float radius;
            float angle;
            float distance;
            float size;
            float amount;
            int direction;
            int shape;
            float center_x, center_y;
            float rotation;
            float zoom_factor;
            
            // Padding
            float padding[16];
        } params;
        
        // Custom curve
        int use_custom_curve;
        std::vector<float> curve_points[4]; // Up to 4 points per dimension
        
        // Padding
        float padding[8];
    };
    
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // Transition textures for complex transitions
    struct TransitionTextures {
        // Noise texture for organic transitions
        VkImage noise_texture;
        VkImageView noise_view;
        VkDeviceMemory noise_memory;
        VkSampler noise_sampler;
        
        bool needs_update = true;
        
        // Gradient textures for wipe transitions
        VkImage gradient_texture;
        VkImageView gradient_view;
        VkDeviceMemory gradient_memory;
        VkSampler gradient_sampler;
        
        bool needs_update = true;
        
        // Pattern textures for custom transitions
        VkImage pattern_texture;
        VkImageView pattern_view;
        VkDeviceMemory pattern_memory;
        VkSampler pattern_sampler;
        
        bool needs_update = true;
    };
    
    TransitionTextures transition_textures_;
    
    // State management
    bool is_playing_ = false;
    std::chrono::steady_clock::time_point start_time_;
    float current_position_ = 0.0f;
    
public:
    TransitionEffect();
    ~TransitionEffect() override;
    
    bool initialize(VkTransitionType type, VkQueue compute_queue) override;
    void cleanup() override;
    
    void set_parameter(const std::string& name, const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value) override;
    
    // Transition control
    void set_transition_params(const TransitionParams& params);
    const TransitionParams& get_transition_params() const { return transition_params_; }
    
    void set_transition_type(TransitionType type);
    void set_duration(float duration);
    void set_position(float position);
    
    void play();
    void pause();
    void stop();
    void reset();
    
    bool is_playing() const { return is_playing_; }
    float get_position() const { return current_position_; }
    
    // Position control
    void seek_to_position(float position);
    void step_position(float delta_time);
    
    // Preset management
    void apply_transition_preset(const std::string& preset_name);
    std::vector<std::string> get_transition_presets() const;
    
private:
    bool create_gpu_resources();
    void destroy_gpu_resources();
    
    bool create_uniform_buffer();
    bool create_descriptor_set();
    bool create_transition_textures();
    
    void update_uniform_buffer();
    void update_transition_textures();
    
    void update_position(float delta_time);
    
    // Transition shaders
    std::string generate_transition_shader(TransitionType type);
    
    // Transition utilities
    float ease_in_out_cubic(float t) const;
    float ease_in_cubic(float t) const;
    float ease_out_cubic(float t) const;
    float ease_in_out_cubic(float t) const;
    
    // Texture generation
    void generate_noise_texture();
    void generate_gradient_texture();
    void generate_pattern_texture(TransitionType type);
};

} // namespace Shotcut::Effects
```

---

## 8.5 REAL-TIME EFFECTS PREVIEW

### 8.5.1 High-Performance Preview System

**Real-Time Preview Philosophy:**
- **60fps Performance:** Smooth preview even with complex effect chains
- **GPU Acceleration:** All preview processing on GPU
- **Optimized Pipeline:** Efficient command buffer management
- **Professional Quality:** Full-resolution preview with minimal quality loss

**src/effects/realtime_preview.h:**
```cpp
#pragma once

#include "effects_framework.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>

namespace Shotcut::Effects {

// Preview settings
struct PreviewSettings {
    glm::ivec2 preview_size = glm::ivec2(1920, 1080);
    float preview_frame_rate = 60.0f;
    
    // Quality settings
    bool use_full_resolution = false;
    float quality_factor = 1.0f; // 0.5 = quarter resolution, 1.0f = full
    
    // Performance settings
    bool use_async_processing = true;
    int max_concurrent_effects = 4;
    int target_frame_time_ms = 16; // 60fps = 16ms per frame
    
    // Buffer management
    bool use_persistent_buffers = true;
    int buffer_pool_size = 3;
};

// Preview frame
struct PreviewFrame {
    VkImage image;
    VkImageView image_view;
    VkDeviceMemory memory;
    VkSampler sampler;
    
    uint32_t width;
    uint32_t height;
    VkFormat format;
    
    std::atomic<bool> in_use{false};
    std::atomic<uint32_t> update_version{0};
    
    PreviewFrame() = default;
    ~PreviewFrame() {
        if (image != VK_NULL_HANDLE) {
            vkDestroyImage(device_, image);
            vkDestroyImageView(device_, image_view);
            vkFreeMemory(device_, memory);
        }
    }
    
    // Disallow copying
    PreviewFrame(const PreviewFrame&) = delete;
    PreviewFrame& operator=(const PreviewFrame&) = delete;
};

// Preview effect instance
struct PreviewEffectInstance {
    std::string effect_id;
    std::shared_ptr<Effect> effect;
    
    std::unordered_map<std::string, std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>> parameters;
    
    bool is_enabled = true;
    int order = 0; // Render order
    
    // Effect chain
    std::vector<std::string> child_effect_ids;
    std::vector<std::shared_ptr<PreviewEffectInstance>> child_instances;
};

// Real-time preview manager
class RealTimePreview {
private:
    // GPU resources
    VkDevice device_;
    VkQueue graphics_queue_;
    uint32_t graphics_queue_family_;
    VkRenderPass render_pass_;
    
    // Command buffer management
    std::vector<VkCommandBuffer> command_buffers_;
    std::vector<VkFence> command_fences_;
    uint32_t current_frame_ = 0;
    uint32_t max_frames_in_flight_;
    
    // Preview settings
    PreviewSettings settings_;
    
    // Frame pool
    std::queue<std::unique_ptr<PreviewFrame>> available_frames_;
    std::vector<std::unique_ptr<PreviewFrame>> active_frames_;
    
    // Preview pipeline
    VkPipeline preview_pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;
    
    // Descriptor management
    VkDescriptorPool descriptor_pool_;
    std::vector<VkDescriptorSet> descriptor_sets_;
    
    // Render resources
    struct RenderResources {
        std::vector<VkBuffer> uniform_buffers;
        std::vector<VkDeviceMemory> uniform_memories;
        
        std::vector<VkBuffer> vertex_buffers;
        std::vector<VkDeviceMemory> vertex_memories;
        std::vector<uint32_t> vertex_counts;
        
        std::vector<VkImage> source_images;
        std::vector<VkImageView> source_image_views;
    };
    
    RenderResources render_resources_;
    
    // Effect management
    std::vector<PreviewEffectInstance> root_effects_;
    std::unordered_map<std::string, std::shared_ptr<PreviewEffectInstance>> all_effects_;
    
    // Preview state
    std::atomic<bool> preview_running_{false};
    std::thread preview_thread_;
    
    // Performance monitoring
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    std::queue<float> frame_times_;
    std::atomic<float> average_frame_time_ms_{0.0f};
    uint32_t frames_rendered_ = 0;
    
public:
    RealTimePreview(VkDevice device, VkQueue graphics_queue, uint32_t graphics_queue_family);
    ~RealTimePreview();
    
    // Initialization
    bool initialize(const PreviewSettings& settings);
    void cleanup();
    
    // Preview control
    void start_preview();
    void pause_preview();
    void stop_preview();
    void reset_preview();
    
    bool is_preview_running() const { return preview_running_.load(); }
    
    // Effect management
    void add_effect(std::shared_ptr<Effect> effect, const std::string& id = "");
    void remove_effect(const std::string& effect_id);
    
    void add_effect_to_chain(const std::string& chain_id, const std::string& effect_id);
    void remove_effect_from_chain(const std::string& chain_id, const std::string& effect_id);
    
    void set_effect_enabled(const std::string& effect_id, bool enabled);
    void set_effect_order(const std::string& effect_id, int order);
    
    void set_effect_parameter(const std::string& effect_id, const std::string& parameter,
                         const std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value);
    
    // Preview settings
    void set_preview_settings(const PreviewSettings& settings);
    const PreviewSettings& get_preview_settings() const { return settings_; }
    
    void set_preview_size(const glm::ivec2& size);
    void set_preview_frame_rate(float fps);
    void set_quality_factor(float factor);
    
    // Frame capture
    std::unique_ptr<PreviewFrame> capture_frame();
    void capture_frame_to_image(const std::string& filename);
    
    // Performance monitoring
    float get_average_frame_time() const { return average_frame_time_ms_.load(); }
    uint32_t get_frames_rendered() const { return frames_rendered_.load(); }
    std::vector<float> get_frame_time_history() const;
    
private:
    bool create_gpu_resources();
    void destroy_gpu_resources();
    
    bool create_render_pass();
    bool create_preview_pipeline();
    bool create_descriptor_sets();
    
    bool acquire_preview_frame(std::unique_ptr<PreviewFrame>& frame);
    void release_preview_frame(std::unique_ptr<PreviewFrame>& frame);
    
    bool setup_render_resources(const std::vector<PreviewEffectInstance>& effects, const PreviewFrame& frame);
    
    void release_render_resources();
    
    // Rendering
    void render_frame_to_swapchain(const PreviewFrame& frame);
    void render_effects_to_frame(const std::vector<PreviewEffectInstance>& effects,
                                    const PreviewFrame& input, PreviewFrame& output);
    
    void preview_thread_function();
    
    // Command buffer management
    VkCommandBuffer begin_command_buffer();
    void end_command_buffer(VkCommandBuffer command_buffer);
    
    // Performance monitoring
    void update_performance_metrics(float frame_time);
    
    // Preview shader
    std::string generate_preview_vertex_shader();
    std::string generate_preview_fragment_shader();
    
    // Frame synchronization
    void wait_for_frame_completion(uint32_t frame_index);
};

// Preview renderer for different effect types
class PreviewRenderer {
private:
    RealTimePreview* preview_;
    
public:
    PreviewRenderer(RealTimePreview* preview);
    virtual ~PreviewRenderer() = default;
    
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    virtual void render_effect(const PreviewEffectInstance& effect, 
                         const PreviewFrame& input, PreviewFrame& output) = 0;
};

class ColorCorrectionPreviewRenderer : public PreviewRenderer {
private:
    struct ColorCorrectionUniforms {
        ColorCorrectionParams params;
        
        // Padding
        float padding[16];
    };
    
public:
    ColorCorrectionPreviewRenderer(RealTimePreview* preview);
    
    bool initialize() override;
    void cleanup() override;
    void render_effect(const PreviewEffectInstance& effect, 
                     const PreviewFrame& input, PreviewFrame& output) override;
};

class BlurSharpenPreviewRenderer : public PreviewRenderer {
private:
    struct BlurSharpenUniforms {
        BlurParams blur;
        SharpenParams sharpen;
        int effect_mode;
        
        // Padding
        float padding[16];
    };
    
public:
    BlurSharpenPreviewRenderer(RealTimePreview* preview);
    
    bool initialize() override;
    void cleanup() override;
    void render_effect(const PreviewEffectInstance& effect, 
                     const PreviewFrame& input, PreviewFrame& output) override;
};

class CompositePreviewRenderer : public PreviewRenderer {
private:
    struct CompositeUniforms {
        CompositeParams composite;
        KeyingParams keying;
        MatteParams matte;
        
        // Padding
        float padding[16];
    };
    
public:
    CompositePreviewRenderer(RealTimePreview* preview);
    
    bool initialize() override;
    void cleanup() override;
    void render_effect(const PreviewEffectInstance& effect, 
                     const PreviewFrame& input, PreviewFrame& output) override;
};

class TransitionPreviewRenderer : public PreviewRenderer {
private:
    struct TransitionUniforms {
        TransitionParams transition;
        float position;
        float padding[16];
    };
    
public:
    TransitionPreviewRenderer(RealTimePreview* preview);
    
    bool initialize() override;
    void cleanup() override;
    void render_effect(const PreviewEffectInstance& effect, 
                     const PreviewFrame& input, PreviewFrame& output) override;
    
    // Transition specific methods
    void update_transition_position(PreviewEffectInstance& effect, float delta_time);
};

} // namespace Shotcut::Effects
```

---

## 8.6 AUDIO PROCESSING EFFECTS

### 8.6.1 Professional Audio Effects System

**Audio Effects Philosophy:**
- **Professional Audio Quality:** Broadcast-quality audio processing
- **Real-Time Performance:** Low-latency audio effects with 60fps preview
- **Industry Standards:** Support for VST3 and professional audio workflows
- **GPU Acceleration:** CUDA-accelerated audio processing where available

**src/effects/audio_effects.h:**
```cpp
#pragma once

#include "effects_framework.h"
#include <vulkan/vulkan.h>
#include <cuda_runtime.h>
#include <memory>
#include <vector>

namespace Shotcut::Effects {

// Audio effect types
enum class AudioEffectType {
    None = 0,
    Equalizer,
    Compressor,
    Limiter,
    Gate,
    Reverb,
    Delay,
    Chorus,
    Flanger,
    Phaser,
    Distortion,
    Pitch,
    TimeStretch,
    SpectrumAnalyzer
};

// Audio effect parameters
struct AudioEffectParams {
    float wet_mix = 0.0f;      // Wet/dry mix
    float output_gain = 0.0f;   // Output gain
    bool bypass = false;        // Effect bypass
    
    // Performance settings
    bool use_gpu_acceleration = true;
    int buffer_size_samples = 1024;
};

// Equalizer parameters
struct EqualizerParams : public AudioEffectParams {
    enum class FilterType {
        Parametric = 0,
        Graphic
    } type = FilterType::Parametric;
    
    enum class FilterShape {
        Bell,
        Bessel,
        Butterworth,
        Chebyshev,
        LinkwitzRiley,
        LinkwitzTransform,
        Shelf
    } shape = FilterShape::Bell;
    
    std::vector<std::pair<float, float>> bands; // frequency, gain
    static constexpr int MAX_BANDS = 32;
    
    // Graphic equalizer
    std::array<glm::vec2, MAX_BANDS> graphic_points;
    bool show_graphic_display = true;
    
    float q_factor = 1.0f;
};

// Compressor parameters
struct CompressorParams : public AudioEffectParams {
    enum class CompressorType {
        Peak = 0,
    RMS,
        Upward,
        Downward,
        Limiter,
        Multiband
    } type = CompressorType::Peak;
    
    float threshold = -20.0f;  // Threshold in dB
    float ratio = 4.0f;        // Ratio
    float attack_time = 0.005f;    // Attack time in seconds
    float release_time = 0.1f;    // Release time in seconds
    float knee_width = 2.0f;        // Knee width in dB
    float makeup_gain = 0.0f;    // Makeup gain in dB
    
    // Advanced settings
    bool use_soft_knee = true;
    bool use_peak_detection = true;
    float lookahead_time = 0.001f;
    float release_cooldown = 0.1f;
    
    // Multiband settings
    int num_bands = 4;
    std::vector<float> band_frequencies;
    std::vector<CompressorParams> band_params;
};

// Audio buffer management
struct AudioBuffer {
    CUdeviceptr device_ptr;
    void* mapped_ptr;
    size_t size_bytes;
    size_t size_samples;
    
    AudioBuffer(size_t sample_count);
    ~AudioBuffer();
    
    void allocate();
    void deallocate();
    
    void copy_from_host(const float* host_data, size_t sample_count);
    void copy_to_host(float* host_data, size_t sample_count);
    void copy_from_device(AudioBuffer& other);
    
    float& operator[](size_t index) {
        return static_cast<float*>(mapped_ptr_)[index];
    }
};

// Audio effect base class
class AudioEffect : public Effect {
private:
    AudioEffectParams params_;
    
    // GPU resources
    VkDevice device_;
    VkQueue compute_queue_;
    uint32_t compute_queue_family_;
    
    // CUDA resources
    CUcontext cuda_context_;
    
    // Audio buffers
    AudioBuffer input_buffer_;
    AudioBuffer output_buffer_;
    AudioBuffer temp_buffer_;
    
    // Effect-specific uniforms
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // Processing state
    bool is_processing_ = false;
    std::atomic<bool> needs_update_{false};
    
public:
    AudioEffect(const std::string& id, const std::string& name, AudioEffectType type);
    virtual ~AudioEffect() = default;
    
    // Effect interface implementation
    bool initialize(VkDevice device, VkQueue compute_queue, 
                uint32_t compute_queue_family,
                CUcontext cuda_context = nullptr) override;
    void cleanup() override;
    
    void set_parameter(const std::string& name, const std::std::variant<float, int, bool, glm::vec2, glm::vec3, glm::vec4>& value) override;
    
    void set_audio_effect_params(const AudioEffectParams& params);
    const AudioEffectParams& get_audio_effect_params() const { return params_; }
    
    // Audio processing
    virtual bool process_audio(const AudioBuffer& input, AudioBuffer& output, 
                                size_t sample_count) = 0;
    
    // Real-time audio processing
    virtual bool process_audio_realtime(const AudioBuffer& input, AudioBuffer& output, 
                                   size_t sample_count, float& output_gain) = 0;
    
protected:
    // Helper functions
    VkDevice get_device() const { return device_; }
    VkQueue get_compute_queue() const { return compute_queue_; }
    uint32_t get_compute_queue_family() const { return compute_queue_family_; }
    
    bool create_audio_buffer(AudioBuffer& buffer, size_t sample_count);
    void destroy_audio_buffer(AudioBuffer& buffer);
    
    bool create_uniform_buffer();
    bool create_descriptor_set();
    
    void update_uniform_buffer();
    void update_audio_buffer(const AudioBuffer& buffer);
    
    // CUDA helper functions
    bool allocate_cuda_memory(void** device_ptr, size_t size);
    void free_cuda_memory(void* device_ptr);
    
    bool copy_to_cuda(const float* host_data, AudioBuffer& cuda_buffer, size_t sample_count);
    bool copy_from_cuda(AudioBuffer& cuda_buffer, float* host_data, size_t sample_count);
    
    void update_processing_state();
};

// Equalizer effect implementation
class EqualizerEffect : public AudioEffect {
private:
    EqualizerParams eq_params_;
    
    // GPU resources for equalizer
    struct EqualizerUniforms {
        // Band parameters
        struct Band {
            float frequency;
            float gain;
            float q_factor;
            
            // Filter parameters
            int filter_type;
            int filter_shape;
            float q;
            
            // Bell filter specific
            float frequency2;
            float bandwidth;
            
            // Padding
            float padding[4];
        } bands[EqualizerParams::MAX_BANDS];
        
        // Graphic equalizer
        glm::vec2 graphic_points[EqualizerParams::MAX_BANDS];
        int show_graphic_display;
        
        // Common parameters
        float wet_mix;
        float output_gain;
        bool bypass;
        int num_bands;
        
        // Performance settings
        int use_gpu_acceleration;
        
        // Padding
        float padding[16];
    };
    
    VkBuffer uniform_buffer_;
    VkDescriptorSet descriptor_set_;
    
    // Frequency response computation
    std::vector<std::array<float, EqualizerParams::MAX_BANDS>> frequency_responses_;
    
    // Biquad filter coefficients
    struct BiquadCoefficients {
        float b0, b1, b2, a0, a1;
        float gain;
    float q;
    float frequency;
        float bandwidth;
    };
    
    std::vector<BiquadCoefficients> filter_coeffs_;
    
public:
    EqualizerEffect();
    ~EqualizerEffect() override;
    
    bool initialize(VkDevice device, VkQueue compute_queue, 
                uint32_t compute_queue_family,
                CUcontext cuda_context = nullptr) override;
    void cleanup() override;
    
    void set_parameter(const std::stream& name, const std::std::variant<float, int, bool, glm::vec2, glm::vec3, glm::
