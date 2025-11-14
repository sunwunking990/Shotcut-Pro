#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

class VulkanContext;
class VulkanSwapchain;

/**
 * @class VulkanGraphicsPipeline
 * @brief Manages Vulkan graphics pipeline for UI rendering
 * 
 * Handles:
 * - Shader compilation and loading
 * - Pipeline creation
 * - Descriptor sets and layouts
 * - Push constants for dynamic data
 */
class VulkanGraphicsPipeline {
public:
    struct Vertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 uv;
        
        static VkVertexInputBindingDescription get_binding_description();
        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
    };

    VulkanGraphicsPipeline(VulkanContext* context, VulkanSwapchain* swapchain);
    ~VulkanGraphicsPipeline();

    // Initialization
    bool initialize();
    void cleanup();

    // Accessors
    VkPipeline pipeline() const { return pipeline_; }
    VkPipelineLayout pipeline_layout() const { return pipeline_layout_; }
    VkDescriptorSetLayout descriptor_set_layout() const { return descriptor_set_layout_; }

    // Shader management
    VkShaderModule create_shader_module(const std::vector<uint32_t>& code);
    void destroy_shader_module(VkShaderModule module);

private:
    VulkanContext* context_;
    VulkanSwapchain* swapchain_;
    VkPipeline pipeline_;
    VkPipelineLayout pipeline_layout_;
    VkDescriptorSetLayout descriptor_set_layout_;

    bool create_pipeline();
    bool create_descriptor_set_layout();
    bool create_pipeline_layout();
};
