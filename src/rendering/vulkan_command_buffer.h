#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class VulkanContext;
class VulkanSwapchain;
class VulkanGraphicsPipeline;

/**
 * @class VulkanCommandBuffer
 * @brief Manages Vulkan command buffers for rendering
 * 
 * Handles:
 * - Command pool creation
 * - Command buffer allocation
 * - Recording rendering commands
 * - Synchronization primitives
 */
class VulkanCommandBuffer {
public:
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanCommandBuffer(VulkanContext* context, VulkanSwapchain* swapchain, VulkanGraphicsPipeline* pipeline);
    ~VulkanCommandBuffer();

    // Initialization
    bool initialize();
    void cleanup();

    // Frame management
    bool begin_frame(uint32_t& image_index);
    bool end_frame();

    // Command recording
    void begin_render_pass(uint32_t image_index);
    void end_render_pass();
    void bind_pipeline();
    void draw_vertices(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0);

    // Accessors
    VkCommandBuffer current_command_buffer() const { return command_buffers_[current_frame_]; }
    uint32_t current_frame() const { return current_frame_; }

private:
    VulkanContext* context_;
    VulkanSwapchain* swapchain_;
    VulkanGraphicsPipeline* pipeline_;

    VkCommandPool command_pool_;
    std::vector<VkCommandBuffer> command_buffers_;
    std::vector<VkSemaphore> image_available_semaphores_;
    std::vector<VkSemaphore> render_finished_semaphores_;
    std::vector<VkFence> in_flight_fences_;
    
    uint32_t current_frame_;
    uint32_t current_image_index_;

    bool create_command_pool();
    bool allocate_command_buffers();
    bool create_sync_objects();
};
