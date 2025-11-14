#include "vulkan_command_buffer.h"
#include "vulkan_context.h"
#include "vulkan_swapchain.h"
#include "vulkan_graphics_pipeline.h"
#include <iostream>
#include <stdexcept>

VulkanCommandBuffer::VulkanCommandBuffer(VulkanContext* context, VulkanSwapchain* swapchain, VulkanGraphicsPipeline* pipeline)
    : context_(context), swapchain_(swapchain), pipeline_(pipeline),
      command_pool_(VK_NULL_HANDLE), current_frame_(0), current_image_index_(0) {
}

VulkanCommandBuffer::~VulkanCommandBuffer() {
    cleanup();
}

bool VulkanCommandBuffer::initialize() {
    try {
        if (!create_command_pool()) {
            return false;
        }
        if (!allocate_command_buffers()) {
            return false;
        }
        if (!create_sync_objects()) {
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Command buffer initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void VulkanCommandBuffer::cleanup() {
    vkDeviceWaitIdle(context_->device());
    
    for (auto semaphore : image_available_semaphores_) {
        vkDestroySemaphore(context_->device(), semaphore, nullptr);
    }
    image_available_semaphores_.clear();
    
    for (auto semaphore : render_finished_semaphores_) {
        vkDestroySemaphore(context_->device(), semaphore, nullptr);
    }
    render_finished_semaphores_.clear();
    
    for (auto fence : in_flight_fences_) {
        vkDestroyFence(context_->device(), fence, nullptr);
    }
    in_flight_fences_.clear();
    
    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(context_->device(), command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }
}

bool VulkanCommandBuffer::create_command_pool() {
    const auto& queue_families = context_->queue_families();
    
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = *queue_families.graphics_family;
    
    VkResult result = vkCreateCommandPool(context_->device(), &pool_info, nullptr, &command_pool_);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create command pool: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanCommandBuffer::allocate_command_buffers() {
    command_buffers_.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());
    
    VkResult result = vkAllocateCommandBuffers(context_->device(), &alloc_info, command_buffers_.data());
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate command buffers: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanCommandBuffer::create_sync_objects() {
    image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result = vkCreateSemaphore(context_->device(), &semaphore_info, nullptr, &image_available_semaphores_[i]);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create image available semaphore: " << result << std::endl;
            return false;
        }
        
        result = vkCreateSemaphore(context_->device(), &semaphore_info, nullptr, &render_finished_semaphores_[i]);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create render finished semaphore: " << result << std::endl;
            return false;
        }
        
        result = vkCreateFence(context_->device(), &fence_info, nullptr, &in_flight_fences_[i]);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create fence: " << result << std::endl;
            return false;
        }
    }
    
    return true;
}

bool VulkanCommandBuffer::begin_frame(uint32_t& image_index) {
    vkWaitForFences(context_->device(), 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
    
    VkResult result = swapchain_->acquire_next_image(image_index, image_available_semaphores_[current_frame_]);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire next image: " << result << std::endl;
        return false;
    }
    
    vkResetFences(context_->device(), 1, &in_flight_fences_[current_frame_]);
    
    vkResetCommandBuffer(command_buffers_[current_frame_], 0);
    
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    result = vkBeginCommandBuffer(command_buffers_[current_frame_], &begin_info);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to begin command buffer: " << result << std::endl;
        return false;
    }
    
    current_image_index_ = image_index;
    return true;
}

bool VulkanCommandBuffer::end_frame() {
    VkResult result = vkEndCommandBuffer(command_buffers_[current_frame_]);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to end command buffer: " << result << std::endl;
        return false;
    }
    
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores_[current_frame_];
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[current_frame_];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semaphores_[current_frame_];
    
    result = vkQueueSubmit(context_->graphics_queue(), 1, &submit_info, in_flight_fences_[current_frame_]);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to submit command buffer: " << result << std::endl;
        return false;
    }
    
    result = swapchain_->present_image(current_image_index_, render_finished_semaphores_[current_frame_]);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to present image: " << result << std::endl;
        return false;
    }
    
    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void VulkanCommandBuffer::begin_render_pass(uint32_t image_index) {
    VkClearValue clear_values[2];
    clear_values[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    clear_values[1].depthStencil = {1.0f, 0};
    
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = swapchain_->render_pass();
    render_pass_info.framebuffer = swapchain_->images()[image_index].framebuffer;
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_->extent();
    render_pass_info.clearValueCount = 2;
    render_pass_info.pClearValues = clear_values;
    
    vkCmdBeginRenderPass(command_buffers_[current_frame_], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::end_render_pass() {
    vkCmdEndRenderPass(command_buffers_[current_frame_]);
}

void VulkanCommandBuffer::bind_pipeline() {
    vkCmdBindPipeline(command_buffers_[current_frame_], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline());
}

void VulkanCommandBuffer::draw_vertices(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
    vkCmdDraw(command_buffers_[current_frame_], vertex_count, instance_count, first_vertex, first_instance);
}
