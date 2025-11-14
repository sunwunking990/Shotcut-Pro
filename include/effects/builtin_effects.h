#pragma once

// Built-in Effects - Professional Video Effects Implementation
// Color correction, filters, transformations, and more

#include "effect_system.h"
#include <memory>

namespace Shotcut::Effects {

// ============================================================================
// COLOR CORRECTION EFFECTS
// ============================================================================

/// Color Correction Effect - Professional color grading
class ColorCorrectionEffect : public EffectDefinition {
public:
    ColorCorrectionEffect();
    std::unique_ptr<EffectProcessor> create_processor() const override;
};

class ColorCorrectionProcessor : public ShaderEffectProcessor {
public:
    ColorCorrectionProcessor(const EffectDefinition& definition);
    
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) override;
    
protected:
    VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const override;
    std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const override;
    std::vector<VkPushConstantRange> create_push_constant_ranges() const override;
    void update_uniform_buffer(const EffectParameters& parameters, float time) override;
    
private:
    struct UniformData {
        float brightness[3];      // RGB brightness
        float contrast[3];         // RGB contrast
        float saturation[3];       // RGB saturation
        float gamma[3];            // RGB gamma
        float lift[3];             // RGB lift (shadows)
        float gain[3];             // RGB gain (highlights)
        float temperature;          // Color temperature
        float tint;                // Color tint
        float hue_shift;           // Hue rotation
        float vibrance;            // Vibrance
        int32_t mode;              // Processing mode
    };
    
    UniformData uniform_data_{};
};

// ============================================================================
// FILTER EFFECTS
// ============================================================================

/// Gaussian Blur Effect - Variable radius blur
class GaussianBlurEffect : public EffectDefinition {
public:
    GaussianBlurEffect();
    std::unique_ptr<EffectProcessor> create_processor() const override;
};

class GaussianBlurProcessor : public ShaderEffectProcessor {
public:
    GaussianBlurProcessor(const EffectDefinition& definition);
    
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) override;
    
protected:
    VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const override;
    std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const override;
    std::vector<VkPushConstantRange> create_push_constant_ranges() const override;
    void update_uniform_buffer(const EffectParameters& parameters, float time) override;
    
private:
    struct UniformData {
        float radius;              // Blur radius
        float sigma;               // Gaussian sigma
        float direction_x;          // Blur direction (for multi-pass)
        float direction_y;
        int32_t kernel_size;       // Kernel size
        float padding[3];
    };
    
    UniformData uniform_data_{};
    
    // Multi-pass blur support
    bool two_pass_ = true;
    VkImage intermediate_image_ = VK_NULL_HANDLE;
    VkImageView intermediate_view_ = VK_NULL_HANDLE;
    VkDeviceMemory intermediate_memory_ = VK_NULL_HANDLE;
};

/// Sharpen Effect - Unsharp mask sharpening
class SharpenEffect : public EffectDefinition {
public:
    SharpenEffect();
    std::unique_ptr<EffectProcessor> create_processor() const override;
};

class SharpenProcessor : public ShaderEffectProcessor {
public:
    SharpenProcessor(const EffectDefinition& definition);
    
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) override;
    
protected:
    VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const override;
    std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const override;
    std::vector<VkPushConstantRange> create_push_constant_ranges() const override;
    void update_uniform_buffer(const EffectParameters& parameters, float time) override;
    
private:
    struct UniformData {
        float amount;              // Sharpen amount
        float radius;              // Sharpen radius
        float threshold;           // Edge threshold
        int32_t mode;             // Sharpen mode
        float padding[3];
    };
    
    UniformData uniform_data_{};
};

// ============================================================================
// TRANSFORM EFFECTS
// ============================================================================

/// Scale Effect - Image scaling with quality options
class ScaleEffect : public EffectDefinition {
public:
    ScaleEffect();
    std::unique_ptr<EffectProcessor> create_processor() const override;
};

class ScaleProcessor : public ShaderEffectProcessor {
public:
    ScaleProcessor(const EffectDefinition& definition);
    
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) override;
    
protected:
    VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const override;
    std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const override;
    std::vector<VkPushConstantRange> create_push_constant_ranges() const override;
    void update_uniform_buffer(const EffectParameters& parameters, float time) override;
    
private:
    struct UniformData {
        float scale_x;            // X scale
        float scale_y;            // Y scale
        float offset_x;           // X offset
        float offset_y;           // Y offset
        float rotation;            // Rotation angle
        int32_t filter_mode;       // Filtering mode
        float padding[2];
    };
    
    UniformData uniform_data_{};
};

// ============================================================================
// GENERATOR EFFECTS
// ============================================================================

/// Noise Generator - Procedural noise generation
class NoiseGeneratorEffect : public EffectDefinition {
public:
    NoiseGeneratorEffect();
    std::unique_ptr<EffectProcessor> create_processor() const override;
};

class NoiseGeneratorProcessor : public ShaderEffectProcessor {
public:
    NoiseGeneratorProcessor(const EffectDefinition& definition);
    
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) override;
    
protected:
    VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const override;
    std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const override;
    std::vector<VkPushConstantRange> create_push_constant_ranges() const override;
    void update_uniform_buffer(const EffectParameters& parameters, float time) override;
    
private:
    struct UniformData {
        float time;                // Animation time
        float scale;               // Noise scale
        float amount;              // Noise amount
        int32_t noise_type;        // Noise type (perlin, simplex, etc.)
        int32_t seed;              // Random seed
        float octaves;             // Noise octaves
        float persistence;         // Persistence
        float lacunarity;          // Lacunarity
        int32_t color_mode;         // Color mode
    };
    
    UniformData uniform_data_{};
};

// ============================================================================
// TRANSITION EFFECTS
// ============================================================================

/// Crossfade Transition - Smooth transition between clips
class CrossfadeTransition : public EffectDefinition {
public:
    CrossfadeTransition();
    std::unique_ptr<EffectProcessor> create_processor() const override;
};

class CrossfadeTransitionProcessor : public ShaderEffectProcessor {
public:
    CrossfadeTransitionProcessor(const EffectDefinition& definition);
    
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) override;
    
protected:
    VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const override;
    std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const override;
    std::vector<VkPushConstantRange> create_push_constant_ranges() const override;
    void update_uniform_buffer(const EffectParameters& parameters, float time) override;
    
private:
    struct UniformData {
        float progress;            // Transition progress (0.0 - 1.0)
        float duration;            // Transition duration
        int32_t curve_type;        // Easing curve
        float padding[1];
    };
    
    UniformData uniform_data_{};
    
    // Need second input image for transitions
    VkDescriptorSetLayout transition_descriptor_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool transition_descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet transition_descriptor_set_ = VK_NULL_HANDLE;
};

// ============================================================================
// BUILTIN EFFECTS REGISTRATION
// ============================================================================

/// Register all built-in effects with the effect manager
void register_builtin_effects();

// Utility functions for shader code generation
namespace shaders {
    std::string generate_color_correction_shader();
    std::string generate_gaussian_blur_shader();
    std::string generate_sharpen_shader();
    std::string generate_scale_shader();
    std::string generate_noise_shader();
    std::string generate_crossfade_shader();
    
    // Utility functions for shader generation
    std::string generate_rgb_to_yuv();
    std::string generate_yuv_to_rgb();
    std::string generate_gamma_correction();
    std::string generate_noise_functions();
}

} // namespace Shotcut::Effects
