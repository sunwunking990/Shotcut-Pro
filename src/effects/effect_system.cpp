// GPU Effects System Implementation
// Professional video effects processing with Vulkan

#include "../include/effects/effect_system.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>

namespace Shotcut::Effects {

// ============================================================================
// EFFECT PARAMETER IMPLEMENTATION
// ============================================================================

EffectParameter::EffectParameter(const std::string& name, ParameterType type)
    : name_(name)
    , type_(type)
    , display_name_(name)
    , current_value_("0")
    , default_value_("0") {
}

bool EffectParameter::validate_value(const std::string& value) const {
    try {
        switch (type_) {
            case ParameterType::FLOAT:
                std::stof(value);
                if (has_range_) {
                    float val = std::stof(value);
                    return val >= min_value_ && val <= max_value_;
                }
                return true;
                
            case ParameterType::INT:
                std::stoi(value);
                if (has_range_) {
                    int val = std::stoi(value);
                    return val >= static_cast<int>(min_value_) && val <= static_cast<int>(max_value_);
                }
                return true;
                
            case ParameterType::BOOL:
                return value == "true" || value == "false" || value == "0" || value == "1";
                
            case ParameterType::STRING:
            case ParameterType::TEXTURE:
                return !value.empty();
                
            default:
                return true;
        }
    } catch (...) {
        return false;
    }
}

std::string EffectParameter::serialize() const {
    std::ostringstream oss;
    oss << name_ << "|" << static_cast<int>(type_) << "|" << current_value_ 
         << "|" << default_value_ << "|" << min_value_ << "|" << max_value_
         << "|" << (has_range_ ? "1" : "0") << "|" << (animated_ ? "1" : "0");
    return oss.str();
}

bool EffectParameter::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    
    if (!std::getline(iss, token, '|')) return false;
    name_ = token;
    
    if (!std::getline(iss, token, '|')) return false;
    type_ = static_cast<ParameterType>(std::stoi(token));
    
    if (!std::getline(iss, token, '|')) return false;
    current_value_ = token;
    
    if (!std::getline(iss, token, '|')) return false;
    default_value_ = token;
    
    if (!std::getline(iss, token, '|')) return false;
    min_value_ = std::stof(token);
    
    if (!std::getline(iss, token, '|')) return false;
    max_value_ = std::stof(token);
    
    if (!std::getline(iss, token, '|')) return false;
    has_range_ = (token == "1");
    
    if (!std::getline(iss, token, '|')) return false;
    animated_ = (token == "1");
    
    return true;
}

// ============================================================================
// EFFECT PARAMETERS IMPLEMENTATION
// ============================================================================

void EffectParameters::add_parameter(std::unique_ptr<EffectParameter> param) {
    const std::string& name = param->get_name();
    parameters_.push_back(std::move(param));
    parameter_map_[name] = parameters_.back().get();
}

EffectParameter* EffectParameters::get_parameter(const std::string& name) const {
    auto it = parameter_map_.find(name);
    return it != parameter_map_.end() ? it->second : nullptr;
}

std::string EffectParameters::serialize() const {
    std::ostringstream oss;
    for (size_t i = 0; i < parameters_.size(); ++i) {
        if (i > 0) oss << ";";
        oss << parameters_[i]->serialize();
    }
    return oss.str();
}

bool EffectParameters::deserialize(const std::string& data) {
    std::istringstream iss(data);
    std::string token;
    
    parameters_.clear();
    parameter_map_.clear();
    
    while (std::getline(iss, token, ';')) {
        auto param = std::make_unique<EffectParameter>("", ParameterType::FLOAT);
        if (param->deserialize(token)) {
            const std::string& name = param->get_name();
            parameters_.push_back(std::move(param));
            parameter_map_[name] = parameters_.back().get();
        }
    }
    
    return true;
}

// ============================================================================
// EFFECT DEFINITION IMPLEMENTATION
// ============================================================================

EffectDefinition::EffectDefinition(const std::string& id, const std::string& name, EffectCategory category)
    : id_(id)
    , name_(name)
    , category_(category) {
}

bool EffectDefinition::validate_parameters(const EffectParameters& params) const {
    // Validate all parameters against their definitions
    for (const auto& param : parameters_.get_parameters()) {
        const std::string& name = param->get_name();
        auto* value_param = params.get_parameter(name);
        
        if (!value_param) {
            return false; // Required parameter missing
        }
        
        // Type compatibility check would go here
        // For now, just validate the value
        if (!param->validate_value(value_param->get_value<std::string>())) {
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// EFFECT PROCESSOR IMPLEMENTATION
// ============================================================================

EffectProcessor::EffectProcessor(const EffectDefinition& definition)
    : definition_(definition) {
}

VkShaderModule EffectProcessor::create_shader_module(VulkanContext* context, const std::vector<uint32_t>& spirv) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = spirv.size() * sizeof(uint32_t);
    create_info.pCode = spirv.data();
    
    VkShaderModule shader_module;
    if (vkCreateShaderModule(context->get_device(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    
    return shader_module;
}

VkDescriptorSetLayout EffectProcessor::create_descriptor_set_layout(VulkanContext* context) {
    // Default implementation - should be overridden
    return VK_NULL_HANDLE;
}

bool EffectProcessor::validate_vulkan_result(VkResult result, const std::string& operation) {
    return result == VK_SUCCESS;
}

// ============================================================================
// SHADER EFFECT PROCESSOR IMPLEMENTATION
// ============================================================================

ShaderEffectProcessor::ShaderEffectProcessor(const EffectDefinition& definition)
    : EffectProcessor(definition) {
}

ShaderEffectProcessor::~ShaderEffectProcessor() {
    cleanup();
}

bool ShaderEffectProcessor::initialize(VulkanContext* vulkan_context) {
    vulkan_context_ = vulkan_context;
    
    if (!create_descriptor_sets()) {
        return false;
    }
    
    if (!create_uniform_buffer()) {
        return false;
    }
    
    if (!create_texture_sampler()) {
        return false;
    }
    
    if (!create_pipeline()) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

void ShaderEffectProcessor::cleanup() {
    cleanup_resources();
    
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(vulkan_context_->get_device(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vulkan_context_->get_device(), pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
    
    initialized_ = false;
}

bool ShaderEffectProcessor::recreate_resources(VulkanContext* vulkan_context, uint32_t width, uint32_t height) {
    if (!initialized_) {
        return initialize(vulkan_context);
    }
    
    if (width != current_width_ || height != current_height_) {
        cleanup_resources();
        
        if (!create_uniform_buffer()) {
            return false;
        }
        
        if (!create_pipeline()) {
            return false;
        }
        
        current_width_ = width;
        current_height_ = height;
    }
    
    return true;
}

VkPipelineShaderStageCreateInfo ShaderEffectProcessor::create_vertex_shader_stage() const {
    // Default vertex shader for full-screen quad
    static const std::vector<uint32_t> vertex_shader_spirv = {
        // Minimal SPIR-V for vertex shader
        0x07230203, 0x00010000, 0x000d000b, 0x00000012, 0x0000002a,
        0x00000001, 0x0000000b, 0x0000000d, 0x0000002f, 0x00000002,
        // ... (would be actual compiled SPIR-V)
    };
    
    VkShaderModule shader_module = create_shader_module(vulkan_context_, vertex_shader_spirv);
    
    VkPipelineShaderStageCreateInfo stage_info{};
    stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage_info.module = shader_module;
    stage_info.pName = "main";
    
    return stage_info;
}

bool ShaderEffectProcessor::create_pipeline() {
    // Create descriptor set layout
    auto bindings = create_descriptor_bindings();
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(vulkan_context_->get_device(), &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        return false;
    }
    
    // Create pipeline layout
    auto push_ranges = create_push_constant_ranges();
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
    pipeline_layout_info.pushConstantRangeCount = static_cast<uint32_t>(push_ranges.size());
    pipeline_layout_info.pPushConstantRanges = push_ranges.data();
    
    if (vkCreatePipelineLayout(vulkan_context_->get_device(), &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        return false;
    }
    
    // Create graphics pipeline (simplified)
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.layout = pipeline_layout_;
    
    // Set shader stages
    VkPipelineShaderStageCreateInfo shader_stages[2];
    shader_stages[0] = create_vertex_shader_stage();
    shader_stages[1] = create_fragment_shader_stage();
    
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    
    // Full implementation would set up all pipeline state
    // For now, this is a placeholder
    
    return true;
}

bool ShaderEffectProcessor::create_descriptor_sets() {
    // Create descriptor pool
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
    };
    
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = 1;
    
    if (vkCreateDescriptorPool(vulkan_context_->get_device(), &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        return false;
    }
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &descriptor_set_layout_;
    
    return vkAllocateDescriptorSets(vulkan_context_->get_device(), &alloc_info, &descriptor_set_) == VK_SUCCESS;
}

bool ShaderEffectProcessor::create_uniform_buffer() {
    // Create uniform buffer for parameters
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = 1024; // Size for uniform data
    buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(vulkan_context_->get_device(), &buffer_info, nullptr, &uniform_buffer_) != VK_SUCCESS) {
        return false;
    }
    
    // Allocate memory (simplified)
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(vulkan_context_->get_device(), uniform_buffer_, &mem_requirements);
    
    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = 0; // Would find appropriate memory type
    
    if (vkAllocateMemory(vulkan_context_->get_device(), &alloc_info, nullptr, &uniform_buffer_memory_) != VK_SUCCESS) {
        return false;
    }
    
    vkBindBufferMemory(vulkan_context_->get_device(), uniform_buffer_, uniform_buffer_memory_, 0);
    
    // Map memory
    vkMapMemory(vulkan_context_->get_device(), uniform_buffer_memory_, 0, buffer_info.size, 0, &uniform_buffer_mapped_);
    
    return true;
}

bool ShaderEffectProcessor::create_texture_sampler() {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy = 1.0f;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = VK_LOD_CLAMP_NONE;
    sampler_info.maxLod = VK_LOD_CLAMP_NONE;
    
    return vkCreateSampler(vulkan_context_->get_device(), &sampler_info, nullptr, &texture_sampler_) == VK_SUCCESS;
}

void ShaderEffectProcessor::cleanup_resources() {
    if (uniform_buffer_mapped_ != nullptr) {
        vkUnmapMemory(vulkan_context_->get_device(), uniform_buffer_memory_);
        uniform_buffer_mapped_ = nullptr;
    }
    
    if (uniform_buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(vulkan_context_->get_device(), uniform_buffer_, nullptr);
        uniform_buffer_ = VK_NULL_HANDLE;
    }
    
    if (uniform_buffer_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(vulkan_context_->get_device(), uniform_buffer_memory_, nullptr);
        uniform_buffer_memory_ = VK_NULL_HANDLE;
    }
    
    if (texture_sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(vulkan_context_->get_device(), texture_sampler_, nullptr);
        texture_sampler_ = VK_NULL_HANDLE;
    }
    
    if (descriptor_set_ != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(vulkan_context_->get_device(), descriptor_pool_, 1, &descriptor_set_);
        descriptor_set_ = VK_NULL_HANDLE;
    }
    
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vulkan_context_->get_device(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
    
    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(vulkan_context_->get_device(), descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

// ============================================================================
// EFFECT CHAIN IMPLEMENTATION
// ============================================================================

EffectChain::EffectChain() = default;

EffectChain::~EffectChain() {
    clear_effects();
}

void EffectChain::add_effect(std::unique_ptr<EffectProcessor> effect, const EffectParameters& parameters) {
    EffectInstance instance;
    instance.processor = std::move(effect);
    instance.parameters = parameters;
    effects_.push_back(std::move(instance));
}

void EffectChain::remove_effect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void EffectChain::clear_effects() {
    effects_.clear();
    cleanup_intermediate_framebuffers();
}

void EffectChain::reorder_effects(const std::vector<size_t>& new_order) {
    std::vector<EffectInstance> reordered;
    reordered.reserve(new_order.size());
    
    for (size_t index : new_order) {
        if (index < effects_.size()) {
            reordered.push_back(std::move(effects_[index]));
        }
    }
    
    effects_ = std::move(reordered);
}

bool EffectChain::initialize(VulkanContext* vulkan_context) {
    vulkan_context_ = vulkan_context;
    
    for (auto& effect : effects_) {
        if (!effect.processor->initialize(vulkan_context)) {
            return false;
        }
    }
    
    initialized_ = true;
    return true;
}

bool EffectChain::process_frame(
    VulkanContext* vulkan_context,
    VkCommandBuffer command_buffer,
    VkImageView input_image,
    VkImageView output_image,
    uint32_t width,
    uint32_t height,
    float time) {
    
    if (!initialized_) {
        return false;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Create intermediate framebuffers if needed
    if (!create_intermediate_framebuffers(width, height)) {
        return false;
    }
    
    VkImageView current_input = input_image;
    VkImageView current_output = nullptr;
    
    // Process each effect in sequence
    for (size_t i = 0; i < effects_.size(); ++i) {
        auto& effect = effects_[i];
        
        // Determine output target
        if (i == effects_.size() - 1) {
            // Last effect - output to final target
            current_output = output_image;
        } else {
            // Intermediate effect - output to intermediate framebuffer
            current_output = intermediate_framebuffers_[i].image_view;
        }
        
        // Process the frame
        if (!effect.processor->process_frame(
                vulkan_context,
                command_buffer,
                current_input,
                current_output,
                width,
                height,
                time,
                effect.parameters)) {
            return false;
        }
        
        current_input = current_output;
    }
    
    // Update performance metrics
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    performance_.total_time_ms = duration.count() / 1000.0f;
    
    return true;
}

void EffectChain::set_quality(EffectQuality quality) {
    for (auto& effect : effects_) {
        effect.processor->set_quality(quality);
    }
}

bool EffectChain::create_intermediate_framebuffers(uint32_t width, uint32_t height) {
    // Clean up existing framebuffers
    cleanup_intermediate_framebuffers();
    
    // Need (effects_count - 1) intermediate framebuffers
    size_t needed_count = effects_.size() > 1 ? effects_.size() - 1 : 0;
    
    if (needed_count == 0) {
        return true;
    }
    
    intermediate_framebuffers_.resize(needed_count);
    
    for (size_t i = 0; i < needed_count; ++i) {
        auto& fb = intermediate_framebuffers_[i];
        
        // Create image
        VkImageCreateInfo image_info{};
        image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = width;
        image_info.extent.height = height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = VK_FORMAT_R8G8B8A8_UNORM; // Would be configurable
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        
        if (vkCreateImage(vulkan_context_->get_device(), &image_info, nullptr, &fb.image) != VK_SUCCESS) {
            return false;
        }
        
        // Create image view (simplified - actual implementation would need memory allocation)
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = fb.image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(vulkan_context_->get_device(), &view_info, nullptr, &fb.image_view) != VK_SUCCESS) {
            return false;
        }
        
        fb.width = width;
        fb.height = height;
    }
    
    return true;
}

void EffectChain::cleanup_intermediate_framebuffers() {
    for (auto& fb : intermediate_framebuffers_) {
        if (fb.image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(vulkan_context_->get_device(), fb.image_view, nullptr);
            fb.image_view = VK_NULL_HANDLE;
        }
        
        if (fb.image != VK_NULL_HANDLE) {
            vkDestroyImage(vulkan_context_->get_device(), fb.image, nullptr);
            fb.image = VK_NULL_HANDLE;
        }
        
        if (fb.memory != VK_NULL_HANDLE) {
            vkFreeMemory(vulkan_context_->get_device(), fb.memory, nullptr);
            fb.memory = VK_NULL_HANDLE;
        }
    }
    
    intermediate_framebuffers_.clear();
}

// ============================================================================
// EFFECT MANAGER IMPLEMENTATION
// ============================================================================

EffectManager& EffectManager::get_instance() {
    static EffectManager instance;
    return instance;
}

void EffectManager::register_effect(std::unique_ptr<EffectDefinition> definition) {
    const std::string& id = definition->get_id();
    effect_definitions_[id] = std::move(definition);
}

void EffectManager::unregister_effect(const std::string& id) {
    effect_definitions_.erase(id);
}

std::vector<std::string> EffectManager::get_effect_ids() const {
    std::vector<std::string> ids;
    ids.reserve(effect_definitions_.size());
    
    for (const auto& pair : effect_definitions_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

std::vector<std::string> EffectManager::get_effects_by_category(EffectCategory category) const {
    std::vector<std::string> ids;
    
    for (const auto& pair : effect_definitions_) {
        if (pair.second->get_category() == category) {
            ids.push_back(pair.first);
        }
    }
    
    return ids;
}

const EffectDefinition* EffectManager::get_effect_definition(const std::string& id) const {
    auto it = effect_definitions_.find(id);
    return it != effect_definitions_.end() ? it->second.get() : nullptr;
}

std::unique_ptr<EffectProcessor> EffectManager::create_effect_processor(const std::string& id) const {
    auto* definition = get_effect_definition(id);
    return definition ? definition->create_processor() : nullptr;
}

void EffectManager::save_preset(const EffectPreset& preset) {
    const std::string& effect_id = preset.effect_id;
    presets_[effect_id].push_back(preset);
}

std::vector<EffectManager::EffectPreset> EffectManager::get_presets(const std::string& effect_id) const {
    auto it = presets_.find(effect_id);
    return it != presets_.end() ? it->second : std::vector<EffectPreset>();
}

bool EffectManager::load_preset(const std::string& preset_name, EffectPreset& preset) const {
    // Search all presets by name
    for (const auto& pair : presets_) {
        for (const auto& stored_preset : pair.second) {
            if (stored_preset.name == preset_name) {
                preset = stored_preset;
                return true;
            }
        }
    }
    
    return false;
}

void EffectManager::initialize_builtin_effects() {
    // Register built-in effects
    // Color Correction, Blur, Sharpen, etc.
    register_builtin_effects();
}

void EffectManager::register_builtin_effects() {
    // This would register all built-in effects
    // For now, placeholder
}

} // namespace Shotcut::Effects
