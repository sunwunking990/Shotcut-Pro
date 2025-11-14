#pragma once

// GPU Effects System - Professional Video Effects Processing
// Modern Vulkan-based effects pipeline with real-time processing

#include "../rendering/vulkan_context.h"
#include "../timeline/entity.h"
#include "../timeline/components.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

namespace Shotcut::Effects {

// Forward declarations
class EffectProcessor;
class ShaderManager;
class EffectParameter;
class EffectChain;

// ============================================================================
// EFFECT TYPES AND DEFINITIONS
// ============================================================================

/// Effect categories for organization
enum class EffectCategory {
    COLOR_CORRECTION,
    TRANSFORM,
    FILTER,
    GENERATOR,
    TRANSITION,
    AUDIO,
    CUSTOM
};

/// Effect processing priority (lower = earlier in chain)
enum class EffectPriority {
    TRANSFORM = 100,      // Position/scale/rotation
    COLOR = 200,         // Color correction
    FILTER = 300,        // Blur, sharpen, etc.
    TRANSITION = 400,    // Transitions between clips
    OUTPUT = 500         // Final output processing
};

/// Effect quality levels for performance scaling
enum class EffectQuality {
    DRAFT = 0,          // Fast preview
    GOOD = 1,           // Real-time editing
    BEST = 2,           // Final render
    CUSTOM = 3          // User-defined
};

// ============================================================================
// EFFECT PARAMETERS
// ============================================================================

/// Parameter types for effect configuration
enum class ParameterType {
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    INT,
    INT2,
    INT3,
    INT4,
    BOOL,
    COLOR,
    TEXTURE,
    ENUM
};

/// Single effect parameter with validation and animation support
class EffectParameter {
public:
    EffectParameter(const std::string& name, ParameterType type);
    virtual ~EffectParameter() = default;

    // Parameter info
    const std::string& get_name() const { return name_; }
    ParameterType get_type() const { return type_; }
    const std::string& get_display_name() const { return display_name_; }
    const std::string& get_description() const { return description_; }
    
    // Value access
    template<typename T>
    T get_value() const;
    
    template<typename T>
    void set_value(const T& value);
    
    // Parameter constraints
    void set_range(float min_val, float max_val) {
        min_value_ = min_val;
        max_value_ = max_val;
        has_range_ = true;
    }
    
    void set_default_value(const std::string& default_val) {
        default_value_ = default_val;
        current_value_ = default_val;
    }
    
    // Animation support
    bool is_animated() const { return animated_; }
    void set_animated(bool animated) { animated_ = animated; }
    
    // Validation
    bool validate_value(const std::string& value) const;
    
    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);

private:
    std::string name_;
    ParameterType type_;
    std::string display_name_;
    std::string description_;
    std::string current_value_;
    std::string default_value_;
    float min_value_ = 0.0f;
    float max_value_ = 1.0f;
    bool has_range_ = false;
    bool animated_ = false;
};

/// Parameter collection for an effect
class EffectParameters {
public:
    void add_parameter(std::unique_ptr<EffectParameter> param);
    EffectParameter* get_parameter(const std::string& name) const;
    
    template<typename T>
    T get_parameter_value(const std::string& name) const;
    
    template<typename T>
    void set_parameter_value(const std::string& name, const T& value);
    
    const std::vector<std::unique_ptr<EffectParameter>>& get_parameters() const {
        return parameters_;
    }
    
    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);

private:
    std::vector<std::unique_ptr<EffectParameter>> parameters_;
    std::unordered_map<std::string, EffectParameter*> parameter_map_;
};

// ============================================================================
// EFFECT DEFINITION
// ============================================================================

/// Base effect definition with metadata and parameters
class EffectDefinition {
public:
    EffectDefinition(const std::string& id, const std::string& name, EffectCategory category);
    virtual ~EffectDefinition() = default;
    
    // Basic info
    const std::string& get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    const std::string& get_description() const { return description_; }
    EffectCategory get_category() const { return category_; }
    EffectPriority get_priority() const { return priority_; }
    
    // Metadata
    void set_description(const std::string& desc) { description_ = desc; }
    void set_version(const std::string& version) { version_ = version; }
    void set_author(const std::string& author) { author_ = author; }
    void set_priority(EffectPriority priority) { priority_ = priority; }
    
    // Parameters
    EffectParameters& get_parameters() { return parameters_; }
    const EffectParameters& get_parameters() const { return parameters_; }
    
    // Factory method
    virtual std::unique_ptr<EffectProcessor> create_processor() const = 0;
    
    // Validation
    virtual bool validate_parameters(const EffectParameters& params) const;

protected:
    std::string id_;
    std::string name_;
    std::string description_;
    EffectCategory category_;
    EffectPriority priority_ = EffectPriority::COLOR;
    std::string version_ = "1.0";
    std::string author_ = "Shotcut";
    EffectParameters parameters_;
};

// ============================================================================
// EFFECT PROCESSOR
// ============================================================================

/// Runtime effect processor that applies effects to frames
class EffectProcessor {
public:
    EffectProcessor(const EffectDefinition& definition);
    virtual ~EffectProcessor() = default;
    
    // Processing
    virtual bool initialize(VulkanContext* vulkan_context) = 0;
    virtual bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time,
        const EffectParameters& parameters
    ) = 0;
    
    // Resource management
    virtual void cleanup() = 0;
    virtual bool recreate_resources(VulkanContext* vulkan_context, uint32_t width, uint32_t height) = 0;
    
    // Quality settings
    virtual void set_quality(EffectQuality quality) { quality_ = quality; }
    EffectQuality get_quality() const { return quality_; }
    
    // Performance info
    struct PerformanceMetrics {
        float processing_time_ms = 0.0f;
        uint32_t texture_bindings = 0;
        uint32_t shader_complexity = 0;  // Approximate instruction count
        bool uses_compute_shader = false;
    };
    
    const PerformanceMetrics& get_performance_metrics() const { return performance_metrics_; }
    
    // State
    bool is_initialized() const { return initialized_; }
    const EffectDefinition& get_definition() const { return definition_; }

protected:
    const EffectDefinition& definition_;
    VulkanContext* vulkan_context_ = nullptr;
    bool initialized_ = false;
    EffectQuality quality_ = EffectQuality::GOOD;
    PerformanceMetrics performance_metrics_;
    
    // Helper methods
    VkShaderModule create_shader_module(VulkanContext* context, const std::vector<uint32_t>& spirv);
    VkDescriptorSetLayout create_descriptor_set_layout(VulkanContext* context);
    bool validate_vulkan_result(VkResult result, const std::string& operation);
};

// ============================================================================
// SHADER-BASED EFFECT PROCESSOR
// ============================================================================

/// Base class for effects implemented with Vulkan shaders
class ShaderEffectProcessor : public EffectProcessor {
public:
    ShaderEffectProcessor(const EffectDefinition& definition);
    virtual ~ShaderEffectProcessor();
    
    bool initialize(VulkanContext* vulkan_context) override;
    void cleanup() override;
    bool recreate_resources(VulkanContext* vulkan_context, uint32_t width, uint32_t height) override;
    
protected:
    // Pipeline creation
    virtual VkPipelineShaderStageCreateInfo create_vertex_shader_stage() const;
    virtual VkPipelineShaderStageCreateInfo create_fragment_shader_stage() const = 0;
    virtual std::vector<VkDescriptorSetLayoutBinding> create_descriptor_bindings() const = 0;
    virtual std::vector<VkPushConstantRange> create_push_constant_ranges() const = 0;
    
    // Parameter handling
    virtual void update_uniform_buffer(const EffectParameters& parameters, float time) = 0;
    
    // Resources
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    
    VkBuffer uniform_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory uniform_buffer_memory_ = VK_NULL_HANDLE;
    void* uniform_buffer_mapped_ = nullptr;
    
    VkSampler texture_sampler_ = VK_NULL_HANDLE;
    
    uint32_t current_width_ = 0;
    uint32_t current_height_ = 0;
    
private:
    bool create_pipeline();
    bool create_descriptor_sets();
    bool create_uniform_buffer();
    bool create_texture_sampler();
    void cleanup_resources();
};

// ============================================================================
// EFFECT CHAIN
// ============================================================================

/// Chain of effects applied in sequence
class EffectChain {
public:
    EffectChain();
    ~EffectChain();
    
    // Chain management
    void add_effect(std::unique_ptr<EffectProcessor> effect, const EffectParameters& parameters);
    void remove_effect(size_t index);
    void clear_effects();
    void reorder_effects(const std::vector<size_t>& new_order);
    
    // Processing
    bool initialize(VulkanContext* vulkan_context);
    bool process_frame(
        VulkanContext* vulkan_context,
        VkCommandBuffer command_buffer,
        VkImageView input_image,
        VkImageView output_image,
        uint32_t width,
        uint32_t height,
        float time
    );
    
    // Quality
    void set_quality(EffectQuality quality);
    
    // State
    size_t get_effect_count() const { return effects_.size(); }
    bool is_initialized() const { return initialized_; }
    
    // Performance
    struct ChainPerformance {
        std::vector<EffectProcessor::PerformanceMetrics> effect_metrics;
        float total_time_ms = 0.0f;
        uint32_t texture_transfers = 0;
    };
    
    const ChainPerformance& get_performance_metrics() const { return performance_; }

private:
    struct EffectInstance {
        std::unique_ptr<EffectProcessor> processor;
        EffectParameters parameters;
    };
    
    std::vector<EffectInstance> effects_;
    VulkanContext* vulkan_context_ = nullptr;
    bool initialized_ = false;
    ChainPerformance performance_;
    
    // Intermediate framebuffers for chaining
    struct FramebufferResources {
        VkImage image = VK_NULL_HANDLE;
        VkImageView image_view = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
    };
    
    std::vector<FramebufferResources> intermediate_framebuffers_;
    
    bool create_intermediate_framebuffers(uint32_t width, uint32_t height);
    void cleanup_intermediate_framebuffers();
};

// ============================================================================
// EFFECT MANAGER
// ============================================================================

/// Central manager for all effects in the system
class EffectManager {
public:
    static EffectManager& get_instance();
    
    // Registration
    void register_effect(std::unique_ptr<EffectDefinition> definition);
    void unregister_effect(const std::string& id);
    
    // Discovery
    std::vector<std::string> get_effect_ids() const;
    std::vector<std::string> get_effects_by_category(EffectCategory category) const;
    const EffectDefinition* get_effect_definition(const std::string& id) const;
    
    // Creation
    std::unique_ptr<EffectProcessor> create_effect_processor(const std::string& id) const;
    
    // Presets
    struct EffectPreset {
        std::string name;
        std::string effect_id;
        EffectParameters parameters;
        std::string description;
    };
    
    void save_preset(const EffectPreset& preset);
    std::vector<EffectPreset> get_presets(const std::string& effect_id) const;
    bool load_preset(const std::string& preset_name, EffectPreset& preset) const;
    
    // Built-in effects
    void initialize_builtin_effects();

private:
    EffectManager() = default;
    std::unordered_map<std::string, std::unique_ptr<EffectDefinition>> effect_definitions_;
    std::unordered_map<std::string, std::vector<EffectPreset>> presets_;
    
    void register_builtin_effects();
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

template<typename T>
T EffectParameter::get_value() const {
    if constexpr (std::is_same_v<T, float>) {
        return std::stof(current_value_);
    } else if constexpr (std::is_same_v<T, int>) {
        return std::stoi(current_value_);
    } else if constexpr (std::is_same_v<T, bool>) {
        return current_value_ == "true" || current_value_ == "1";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return current_value_;
    }
    // Add more type specializations as needed
    return T{};
}

template<typename T>
void EffectParameter::set_value(const T& value) {
    if constexpr (std::is_same_v<T, float>) {
        current_value_ = std::to_string(value);
    } else if constexpr (std::is_same_v<T, int>) {
        current_value_ = std::to_string(value);
    } else if constexpr (std::is_same_v<T, bool>) {
        current_value_ = value ? "true" : "false";
    } else if constexpr (std::is_same_v<T, std::string>) {
        current_value_ = value;
    }
    // Add more type specializations as needed
}

template<typename T>
T EffectParameters::get_parameter_value(const std::string& name) const {
    auto it = parameter_map_.find(name);
    if (it != parameter_map_.end()) {
        return it->second->get_value<T>();
    }
    return T{};
}

template<typename T>
void EffectParameters::set_parameter_value(const std::string& name, const T& value) {
    auto it = parameter_map_.find(name);
    if (it != parameter_map_.end()) {
        it->second->set_value(value);
    }
}

} // namespace Shotcut::Effects
