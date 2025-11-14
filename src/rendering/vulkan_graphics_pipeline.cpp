#include "vulkan_graphics_pipeline.h"
#include "vulkan_context.h"
#include "vulkan_swapchain.h"
#include <iostream>
#include <stdexcept>

// Simple vertex shader (compiled to SPIR-V)
// This is a minimal shader for rendering colored rectangles
static const uint32_t vertex_shader_code[] = {
    0x07230203, 0x00010300, 0x0008000b, 0x00000020, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d, 0x0000001b, 0x00030010,
    0x00000004, 0x00000007, 0x00040047, 0x0000000d, 0x0000000b, 0x00000000, 0x00040047, 0x0000001b,
    0x0000000b, 0x00000001, 0x00040047, 0x0000001c, 0x0000000b, 0x00000000, 0x00040047, 0x0000001d,
    0x0000000b, 0x00000001, 0x00040047, 0x0000001e, 0x0000000b, 0x00000002, 0x00040047, 0x0000001f,
    0x0000000b, 0x00000003, 0x00030047, 0x00000002, 0x00000002, 0x00040015, 0x00000006, 0x00000020,
    0x00000001, 0x0003001d, 0x00000007, 0x00000006, 0x0004001e, 0x00000008, 0x00000007, 0x00000006,
    0x00040020, 0x00000009, 0x00000003, 0x00000008, 0x0004003b, 0x00000009, 0x0000000a, 0x00000003,
    0x00040015, 0x0000000b, 0x00000020, 0x00000000, 0x0004002b, 0x0000000b, 0x0000000c, 0x00000000,
    0x00040020, 0x0000000e, 0x00000001, 0x00000006, 0x0004003b, 0x0000000e, 0x0000000d, 0x00000001,
    0x0004002b, 0x00000006, 0x00000010, 0x00000000, 0x00040020, 0x00000011, 0x00000003, 0x00000006,
    0x0004002b, 0x0000000b, 0x00000013, 0x00000001, 0x00040020, 0x00000014, 0x00000001, 0x00000006,
    0x0004003b, 0x00000014, 0x00000015, 0x00000001, 0x00040020, 0x00000016, 0x00000003, 0x00000006,
    0x00040020, 0x00000017, 0x00000001, 0x00000006, 0x0004003b, 0x00000017, 0x00000018, 0x00000001,
    0x0004003b, 0x00000017, 0x00000019, 0x00000001, 0x0004003b, 0x00000017, 0x0000001a, 0x00000001,
    0x00040020, 0x0000001b, 0x00000003, 0x00000006, 0x0004003b, 0x0000001b, 0x0000001c, 0x00000003,
    0x0004003b, 0x0000001b, 0x0000001d, 0x00000003, 0x0004003b, 0x0000001b, 0x0000001e, 0x00000003,
    0x0004003b, 0x0000001b, 0x0000001f, 0x00000003, 0x00050036, 0x00000006, 0x00000004, 0x00000000,
    0x00000005, 0x000200f8, 0x00000020, 0x0004003d, 0x00000006, 0x00000021, 0x0000000d, 0x00050041,
    0x00000011, 0x00000022, 0x0000000a, 0x0000000c, 0x0003003e, 0x00000022, 0x00000021, 0x0004003d,
    0x00000006, 0x00000023, 0x00000015, 0x00050041, 0x00000016, 0x00000024, 0x0000001c, 0x0000000c,
    0x0003003e, 0x00000024, 0x00000023, 0x0004003d, 0x00000006, 0x00000025, 0x00000018, 0x00050041,
    0x00000016, 0x00000026, 0x0000001d, 0x0000000c, 0x0003003e, 0x00000026, 0x00000025, 0x0004003d,
    0x00000006, 0x00000027, 0x00000019, 0x00050041, 0x00000016, 0x00000028, 0x0000001e, 0x0000000c,
    0x0003003e, 0x00000028, 0x00000027, 0x0004003d, 0x00000006, 0x00000029, 0x0000001a, 0x00050041,
    0x00000016, 0x0000002a, 0x0000001f, 0x0000000c, 0x0003003e, 0x0000002a, 0x00000029, 0x000100fd,
    0x00010038
};

// Simple fragment shader (compiled to SPIR-V)
static const uint32_t fragment_shader_code[] = {
    0x07230203, 0x00010300, 0x0008000b, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000d, 0x00030010,
    0x00000004, 0x00000007, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000d,
    0x0000001e, 0x00000000, 0x00030047, 0x00000002, 0x00000002, 0x00040015, 0x00000006, 0x00000020,
    0x00000001, 0x0004002b, 0x00000006, 0x00000007, 0x00000000, 0x0004001c, 0x00000008, 0x00000006,
    0x00000004, 0x00040020, 0x00000009, 0x00000003, 0x00000008, 0x0004003b, 0x00000009, 0x0000000a,
    0x00000003, 0x00040015, 0x0000000b, 0x00000020, 0x00000000, 0x0004002b, 0x0000000b, 0x0000000c,
    0x00000000, 0x00040020, 0x0000000d, 0x00000001, 0x00000008, 0x0004003b, 0x0000000d, 0x0000000e,
    0x00000001, 0x00050036, 0x00000006, 0x00000004, 0x00000000, 0x00000005, 0x000200f8, 0x0000001e,
    0x0004003d, 0x00000008, 0x0000000f, 0x0000000e, 0x00050041, 0x00000009, 0x00000010, 0x0000000a,
    0x0000000c, 0x0003003e, 0x00000010, 0x0000000f, 0x000100fd, 0x00010038
};

VkVertexInputBindingDescription VulkanGraphicsPipeline::Vertex::get_binding_description() {
    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = sizeof(Vertex);
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding_description;
}

std::vector<VkVertexInputAttributeDescription> VulkanGraphicsPipeline::Vertex::get_attribute_descriptions() {
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions(3);
    
    // Position
    attribute_descriptions[0].binding = 0;
    attribute_descriptions[0].location = 0;
    attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[0].offset = offsetof(Vertex, position);
    
    // Color
    attribute_descriptions[1].binding = 0;
    attribute_descriptions[1].location = 1;
    attribute_descriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attribute_descriptions[1].offset = offsetof(Vertex, color);
    
    // UV
    attribute_descriptions[2].binding = 0;
    attribute_descriptions[2].location = 2;
    attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_descriptions[2].offset = offsetof(Vertex, uv);
    
    return attribute_descriptions;
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanContext* context, VulkanSwapchain* swapchain)
    : context_(context), swapchain_(swapchain), pipeline_(VK_NULL_HANDLE),
      pipeline_layout_(VK_NULL_HANDLE), descriptor_set_layout_(VK_NULL_HANDLE) {
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
    cleanup();
}

bool VulkanGraphicsPipeline::initialize() {
    try {
        if (!create_descriptor_set_layout()) {
            return false;
        }
        if (!create_pipeline_layout()) {
            return false;
        }
        if (!create_pipeline()) {
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Graphics pipeline initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void VulkanGraphicsPipeline::cleanup() {
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(context_->device(), pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
    
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context_->device(), pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
    
    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context_->device(), descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

bool VulkanGraphicsPipeline::create_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;
    
    VkResult result = vkCreateDescriptorSetLayout(context_->device(), &layout_info, nullptr, &descriptor_set_layout_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanGraphicsPipeline::create_pipeline_layout() {
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
    
    VkResult result = vkCreatePipelineLayout(context_->device(), &pipeline_layout_info, nullptr, &pipeline_layout_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanGraphicsPipeline::create_pipeline() {
    VkShaderModule vert_shader_module = create_shader_module(
        std::vector<uint32_t>(vertex_shader_code, vertex_shader_code + sizeof(vertex_shader_code) / sizeof(uint32_t))
    );
    VkShaderModule frag_shader_module = create_shader_module(
        std::vector<uint32_t>(fragment_shader_code, fragment_shader_code + sizeof(fragment_shader_code) / sizeof(uint32_t))
    );
    
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";
    
    VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};
    
    auto binding_description = Vertex::get_binding_description();
    auto attribute_descriptions = Vertex::get_attribute_descriptions();
    
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &binding_description;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_->extent().width;
    viewport.height = (float)swapchain_->extent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_->extent();
    
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                           VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable = VK_FALSE;
    
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDepthStencilState = &depth_stencil;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = swapchain_->render_pass();
    pipeline_info.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(context_->device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline_);
    
    destroy_shader_module(vert_shader_module);
    destroy_shader_module(frag_shader_module);
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline: " << result << std::endl;
        return false;
    }
    
    return true;
}

VkShaderModule VulkanGraphicsPipeline::create_shader_module(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size() * sizeof(uint32_t);
    create_info.pCode = code.data();
    
    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(context_->device(), &create_info, nullptr, &shader_module);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    
    return shader_module;
}

void VulkanGraphicsPipeline::destroy_shader_module(VkShaderModule module) {
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(context_->device(), module, nullptr);
    }
}
